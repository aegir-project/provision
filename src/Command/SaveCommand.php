<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Application;
use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Symfony\Component\Console\Exception\InvalidOptionException;
use Symfony\Component\Console\Input\ArrayInput;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class SaveCommand
 *
 * @package Aegir\Provision\Command
 */
class SaveCommand extends Command
{
    
    /**
     * @var string
     */
    private $context_type;

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
          ->setName('save')
          ->setDescription('Save Provision Context.')
          ->setHelp(
            'Saves a ProvisionContext object to file. Currently just passes to "drush provision-save".'
          )
          ->setDefinition($this->getCommandDefinition());
    }

    /**
     * Generate the list of options derived from ProvisionContextType classes.
     *
     * @return \Symfony\Component\Console\Input\InputDefinition
     */
    protected function getCommandDefinition()
    {
        $inputDefinition = [];
        $inputDefinition[] = new InputArgument(
          'context_name',
          InputArgument::REQUIRED,
          'Context to save'
        );
        $inputDefinition[] = new InputOption(
          'context_type',
          null,
          InputOption::VALUE_OPTIONAL,
          'server, platform, or site'
        );
        $inputDefinition[] = new InputOption(
          'delete',
          null,
          InputOption::VALUE_NONE,
          'Remove the alias.'
        );

      // Load all Aegir\Provision\Context and inject their options.
      // @TODO: Use CommandFileDiscovery to include all classes that inherit Aegir\Provision\Context
      $contexts[] = SiteContext::class;
      $contexts[] = PlatformContext::class;
      $contexts[] = ServerContext::class;

      // For each context type...
      foreach ($contexts as $Context) {

          // Load serviceRequirements into input options.
          $all_services = $Context::getServiceOptions();
          foreach ($Context::serviceRequirements() as $type) {
              $option = "server_{$type}";
              $description = $Context::TYPE . ": " . $all_services[$type];
              $inputDefinition[] = new InputOption($option, NULL, InputOption::VALUE_OPTIONAL, $description);
          }

          // Load option_documentation() into input options.
          foreach ($Context::option_documentation() as $option => $description) {
          $inputDefinition[] = new InputOption($option, NULL, InputOption::VALUE_OPTIONAL, $description);
        }
      }
      return new InputDefinition($inputDefinition);
    }
    
    /**
     * Load commonly used properties context_name and context_type.
     *
     * @param \Symfony\Component\Console\Input\InputInterface   $input
     * @param \Symfony\Component\Console\Output\OutputInterface $output
     */
    protected function initialize(InputInterface $input, OutputInterface $output)
    {
        parent::initialize($input,$output);
        $this->context_name = $input->getArgument('context_name');
        $this->context_type = $input->getOption('context_type');
    }
    
    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        if (empty($this->context)) {
            $this->io->comment("No context named '$this->context_name'. Creating a new one...");

            if (empty($this->input->getOption('context_type'))) {
                $context_type = $this->io->choice('Context Type?', [
                    'server',
                    'platform',
                    'site'
                ]);
                $this->input->setOption('context_type', $context_type);
            }
            else {
                $context_type = $this->input->getOption('context_type');
            }

            // Check for context type service requirements.
            $exit = FALSE;
            $this->io->comment("Checking service requirements for context type {$context_type}...");
            $reqs = Application::checkServiceRequirements($context_type);
            foreach ($reqs as $service => $available) {
                if ($available) {
                    $this->io->successLite("Service $service: Available");
                }
                else {
                    $this->io->warningLite("There is no server that provides the service '$service'.");
                    $exit = TRUE;
                }
            }

            if ($exit) {
                $this->io->error('Service requirements are unfulfillable. Please create a new server (provision save) or add to an existing server (provision services).');
                exit(1);
            }


            $properties = $this->askForContextProperties();
            $class = Context::getClassName($this->input->getOption('context_type'));
            $this->context = new $class($input->getArgument('context_name'), $this->getApplication(), $properties);
        }

        // Delete context config.
        if ($input->getOption('delete')) {
            if (!$input->isInteractive() || $this->io->confirm("Delete context '{$this->context_name}' configuration ($this->context->config_path)?")) {
                if ($this->context->deleteConfig()) {
                    $this->io->info('Context file deleted.');
                    exit(0);
                }
                else {
                    $this->io->error('Unable to delete ' . $this->context->config_path);
                    exit(1);
                }
            }
        }

        foreach ($this->context->getProperties() as $name => $value) {
            $rows[] = [$name, $value];
        }
        
        $this->io->table(['Saving Context:', $this->context->name], $rows);
        
        if ($this->io->confirm("Write configuration for <question>{$this->context->type}</question> context <question>{$this->context->name}</question> to <question>{$this->context->config_path}</question>?")) {
            if ($this->context->save()) {
                $this->io->success("Configuration saved to {$this->context->config_path}");
            }
            else {
                $this->io->error("Unable to save configuration to {$this->context->config_path}. ");
            }
        }
        else {
            $this->io->warningLite('Context not saved.');
            return;
        }
//        $command = 'drush provision-save '.$input->getArgument('context_name');
//        $this->process($command);

        // Offer to add services.
        if ($this->input->isInteractive()) {
            $command = $this->getApplication()->find('services');
            $arguments = [
                'context_name' => $this->input->getArgument('context_name'),
                'sub_command' => 'add',
            ];
            while ($this->io->confirm('Add a service?')) {

                $greetInput = new ArrayInput($arguments);
                $returnCode = $command->run($greetInput, $output);
                $returnCodes[$returnCode] = $returnCode;
            }
        }
    }

    /**
     * Override  to add options
     * @param string $question
     */
    public function askForContext($question = 'Choose a context')
    {
        $options = $this->getApplication()->getAllContextsOptions();

        // If there are options, add "new" to the list.
        if (count($options)) {
            $options['new'] = 'Create a new context.';
            $this->context_name = $this->io->choice($question, $options);

            if ($this->context_name == 'new') {
                $this->context_name = $this->io->ask('Context name');
            }
        }
        // If there are no options, just ask for the name to create.
        else {
            $this->context_name = $this->io->ask('Context name');
        }
    }

        /**
     * Loop through this context type's option_documentation() method and ask for each property.
     *
     * @return array
     */
    private function askForContextProperties() {
        if (empty($this->input->getOption('context_type'))) {
            throw new InvalidOptionException('context_type option is required.');
        }
        
        $this->io->comment("Please input context properties.");
    
        $class = '\Aegir\Provision\Context\\' . ucfirst($this->input->getOption('context_type')) . "Context";
        $options = $class::option_documentation();
        $properties = [];
        foreach ($options as $name => $description) {
          
            // If option does not exist, ask for it.
            if (!empty($this->input->getOption($name))) {
                $properties[$name] = $this->input->getOption($name);
                $this->io->comment("Using option {$name}={$properties[$name]}");
            }
            else {
                $properties[$name] = $this->io->ask("$name ($description)");
            }
        }
        return $properties;
    }
}

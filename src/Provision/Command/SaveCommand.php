<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Application;
use Aegir\Provision\Command;
use Aegir\Provision\Console\ProvisionStyle;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Aegir\Provision\Property;
use Aegir\Provision\Provision;
use Aegir\Provision\Service;
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
     * @var bool
     */
    private $newContext = FALSE;

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
            ->setName('save')
            ->setAliases(['add'])
            ->setDescription('Create or update a site, platform, or server.')
            ->setHelp(
                'Use this command to interactively setup a new site, platform or server (known as "contexts"). Metadata is saved to .yml files in the "config_path" folder. Once you have create a context, use the `provision status` command to view the list of added contexts.'
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
        $inputDefinition = ServicesCommand::getCommandOptions();
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

        $inputDefinition[] = new InputOption(
          'verify',
          null,
          InputOption::VALUE_NONE,
          'Run a verify command after saving the context.'
        );

        $inputDefinition[] = new InputOption(
            'ask-defaults',
            'a',
            InputOption::VALUE_NONE,
            'Ask for property values even if a default is provided.'
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
          
          // Load contextRequirements into input options.
          $this_type = $Context::TYPE;
          foreach ($Context::contextRequirements() as $option => $context_type) {
              $description = "{$this_type}: {$option} context name (Must be type: {$context_type}.)";
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
        $this->context_name = $input->getOption('context');
        $this->context_type = $input->getOption('context_type');
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $context_type = $input->getOption('context_type');

        // If this is a new context...
        if (empty($this->context)) {

            $this->newContext = TRUE;

            // If context_type is still empty, throw an exception. Happens if using -n
            if (empty($context_type)) {
                throw new \Exception('Option --context_type must be specified.');
            }
            else {
                $this->input->setOption('context_type', $context_type);
            }
    
            // Handle invalid context_type.
            if (!class_exists(Context::getClassName($context_type))) {
                $types = Context::getContextTypeOptions();
                throw new \Exception(strtr("Context type !type is invalid. Valid options are: !types", [
                    '!type' => $context_type,
                    '!types' => implode(", ", array_keys($types))
                ]));
        
            }

            // Check for context type service requirements.
            $exit = FALSE;
            $reqs = $this->getProvision()->checkServiceRequirements($context_type);
            if ($reqs) {
                $this->io->block("Checking service requirements for context type {$context_type}...");
                foreach ($reqs as $service => $available) {
                    if ($available) {
                        $this->io->successLite("Service $service: Available");
                    }
                    else {
                        $this->io->warningLite("There is no server that provides the service '$service'.");
                        $exit = TRUE;
                    }
                }
            }


            if ($exit) {
                $this->io->error('Service requirements are unfulfillable. Please create a new server (provision save) or add to an existing server (provision services).');
                exit(1);
            }

            // Pass platform options into Site Options
            $this->loadPlatformProperties();

            $options = $this->askForContextProperties();
            $options['name'] = $this->context_name;
            $options['type'] = $this->context_type;
            
            $class = Context::getClassName($this->input->getOption('context_type'));
            $this->context = new $class($input->getOption('context'), $this->getProvision(), $options);
        }
        else {
            $this->getProvision()->io()->helpBlock("Editing context {$this->context->name}...", ProvisionStyle::ICON_EDIT);

            // Save over existing contexts.
            $this->newContext = FALSE;
            $this->input->setOption('context_type', $this->context->type);
            $properties = $this->askForContextProperties();

            // Write over each property with new values.
            foreach ($properties as $name => $value) {
                $this->context->setProperty($name, $value);
            }

            $context_type = $this->context->type;
            $this->input->setOption('context_type', $this->context->type);

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

        $this->context->preSave();

        foreach ($this->context->getProperties() as $name => $value) {
            if ($name == 'services' || $name == 'service_subscriptions') {
                continue;
            }
            $value = is_array($value)? implode(', ', $value): $value;
            $rows[] = [$name, $value];
        }

        $this->io->table(['Saving Context:', $this->context->name], $rows);
        
        if ($this->io->confirm("Save <comment>{$this->context->type}</comment> context <comment>{$this->context->name}</comment> to <fg=white>{$this->context->config_path}</>?")) {
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
//        $command = 'drush provision-save '.$input->getOption('context');
//        $this->process($command);

        // If editing a context, exit here.
        if (!$this->newContext) {
            return;
        }

        // Offer to add services.
        if ($context_type == 'server') {
            $this->askForServices();
        }
        else {
            $this->askForServiceSubscriptions();
        }

        // Offer to verify. (only if --verify option was specified or is interactive and confirmation is made.
        if ($this->input->getOption('verify') || ($this->input->isInteractive() && $this->io->confirm('Would you like to run `provision verify` on this ' . $this->input->getOption('context_type') . '?'))) {
            $command = $this->getApplication()->find('verify');
            $arguments['--context'] = $this->context_name;
            $input = new ArrayInput($arguments);
            exit($command->run($input, $this->output));
        }

    }

    /**
     * Takes --platform option, loads it, and parses properties from that into
     * input options for the site.
     */
    private function loadPlatformProperties() {
        if ($this->context_type == 'site') {
            if ($this->input->getOption('platform') && $platform = $this->getProvision()->getContext($this->input->getOption('platform'))) {

                // Convert HTTP server to server_http option.
                if ($platform->hasService('http')) {
                    $this->input->setOption('server_http', $platform->service('http')->provider->name);
                }

                // Convert all platform properties to $input options.
                foreach ($platform->getProperties() as $name => $value) {
                    if ($name != 'name' && $name != 'type' && $this->input->hasOption($name)) {
                        $this->getProvision()->getLogger()->notice("Setting option '{name}' from platform to '{value}'.", [
                            'name' => $name,
                            'value' => $value,
                        ]);

                        // Detect empty values, and pass FALSE instead.
                        // If we don't, $this->>askForContextProperties() will
                        // not see the option and will ask for it.
                        if (empty($value)) {
                            $value = FALSE;
                        }
                        $this->input->setOption($name, $value);
                    }
                }
            }
        }
    }

    /**
     * Override  to add options
     * @param string $question
     */
    public function askForContext($question = 'Choose a context')
    {
        $options = $this->getProvision()->getAllContextsOptions();

        // If there are options, add "new" to the list.
        if (count($options)) {
            $options['new'] = 'Create a new context.';
            $this->context_name = $this->io->choice($question, $options);

            if ($this->context_name == 'new') {

                if (empty($this->input->getOption('context_type'))) {
                    $type_options = Context::getContextTypeOptions();
                    $context_type = $this->io->choice('Context Type?', $type_options);
                }
                else {
                    $context_type = $this->input->getOption('context_type');
                }
                $this->input->setOption('context_type', $context_type);
                $this->context_name = $this->io->ask('Context name');
            }
        }
        // If there are no options, just ask for the name to create.
        else {
//
//            // FIRST CONTEXT!
//            // @TODO: Move this to it's own class and methods for onboarding.
//            $this->io->title('Welcome to Provision!');
//
//            $this->io->block([
//                "The first context you need to create is a server. It is recommended to call this server 'server_master' but you can call it whatever you'd like.",
//            ]);
//
//            $this->io->writeln([
//                " <fg=blue>Tip: When Provision asks you <info>a question</info>, it may provide a [<comment>default value</comment>].",
//                "      If you just hit enter, that default value will be used.</>"
//            ]);
            $this->input->setOption('context_type', 'server');

            $this->context_name = $this->io->ask('Context name', 'server_master');
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

        $class = Context::getClassName($this->input->getOption('context_type'));
        $options = $class::option_documentation();
        $properties = $this->askForRequiredContexts();
        foreach ($options as $name => $property) {

            if (!empty($properties[$name])) {
                continue;
            }
            
            // Convert string into Property object.
            // Allows option_documentation to return array of strings for simple properties.
            if ( !$property instanceof Property) {
                $property = Provision::newProperty($property);
            }

            // If we are editing a context, override the default property.
            if (!$this->newContext && $current_value = $this->context->getProperty($name)) {
                $property->default = $current_value;
            }

            // If option does not exist, ask for it.  option is FALSE if loaded from platform with empty property. Prevents console from asking for it if empty.
            if (!empty($this->input->getOption($name)) || $this->input->getOption($name) === FALSE) {
                $properties[$name] = $this->input->getOption($name);
                $this->io->comment("Using option {$name}={$properties[$name]}");
            }
            else {

                // If --ask-defaults is not set and there is a default, use it and do not ask the user.
                if ($property->hidden || !$property->forceAsk && !$this->input->getOption('ask-defaults') && !empty($property->default)) {
                    $properties[$name] = $property->default;
                    $this->io->comment("Using default option {$name}={$properties[$name]}");
                }
                // If user has specifically asked to be asked with the --ask-defaults option, then ask for it.
                else {
                    $properties[$name] = $this->io->ask("{$name} ({$property->description})", $property->default, $property->validate);
                }
            }
        }
        return $properties;
    }

    /**
     * When a server is being added, offer to add services to it interactively.
     */
    protected function askForServices() {
        if (!$this->input->isInteractive()){
            return;
        }
        $command = $this->getApplication()->find('services');
        $arguments = [
            '--context' => $this->input->getOption('context'),
            'sub_command' => 'add',
        ];
        while ($this->io->confirm('Add a service?')) {

            $greetInput = new ArrayInput($arguments);
            $returnCode = $command->run($greetInput, $this->output);
            $returnCodes[$returnCode] = $returnCode;
        }
    }

    /**
     * After a Service Subscriber is added, offer to setup service subscriptions.
     */
    protected function askForServiceSubscriptions() {

        // Lookup servers.
        $all_services = Context::getServiceOptions();
        $class = Context::getClassName($this->input->getOption('context_type'));
        foreach ($class::serviceRequirements() as $type) {
            $option = "server_{$type}";
//            else {
//                $context_name = $this->io->ask($all_services[$type]);
//            }

//            $context = Provision::getContext($context_name);

            $this->io->info("Adding required service $type...");

            $command = $this->getApplication()->find('services');
            $arguments = [
                '--context' => $this->input->getOption('context'),
                'sub_command' => 'add',
                'service' => $type,
            ];

            // Pass option down to services command.
            if (!empty($this->input->getOption($option))) {
                $arguments['server'] = $this->input->getOption($option);
            }

            // If server_http is not specified, but it exists in the platform use that.

            // Pass all options for this service to the services command.
            $service_class = Service::getClassName($type);
            $options_method = $this->input->getOption('context_type') . "_options";
            foreach ($service_class::$options_method() as $option => $help) {
                $arguments["--{$option}"] = $this->input->getOption($option);
            }

            $input = new ArrayInput($arguments);
            $returnCode = $command->run($input, $this->output);
            $returnCodes[$returnCode] = $returnCode;

//            if ($context::TYPE != 'server') {
//                throw new \Exception("Specified context '{$context->name}' is not a server.");
//            }
//            $this->io->comment("Using server $context_name for service $type.");
        }
    }
    
    private function askForRequiredContexts() {
        $contexts = [];
        $class = Context::getClassName($this->input->getOption('context_type'));
        foreach ($class::contextRequirements() as $property => $context_type) {
            
            if ($this->input->getOption($property)) {
                $contexts[$property] = $this->input->getOption($property);
                
                try {
                    $context = $this->getProvision()->getContext($contexts[$property]);
                }
                catch (\Exception $e) {
                    throw new \Exception("Context set by option --{$property} does not exist.");
                }
                
                if ($context->type != $context_type){
                    throw new \Exception("Context set by option --{$property} is a {$context->type}, should be of type {$context_type}.");
                }
            }
            else {
                $contexts[$property] = $this->io->choice("Select $property context", $this->getProvision()->getAllContextsOptions($context_type));
            }
        }
        return $contexts;
    }
}

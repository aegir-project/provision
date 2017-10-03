<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
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
          InputOption::VALUE_OPTIONAL,
          'Remove the alias.'
        );

      // Load all Aegir\Provision\Context and inject their options.
      // @TODO: Figure out a way to do discovery to include all classes that inherit Aegir\Provision\Context
      $contexts[] = SiteContext::option_documentation();
      $contexts[] = PlatformContext::option_documentation();
      $contexts[] = ServerContext::option_documentation();
      
      foreach ($contexts as $context_options) {
        foreach ($context_options as $option => $description) {
          $inputDefinition[] = new InputOption($option, NULL, InputOption::VALUE_OPTIONAL, $description);
        }
      }
      
      return new InputDefinition($inputDefinition);
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        try {
            $context = $this->getApplication()->getContext($input->getArgument('context_name'));
        }
        catch (\Exception $e) {

            $context_name = $this->input->getArgument('context_name');
            $this->io->comment("No context named '$context_name'. Creating a new one...");

            if (empty($this->input->getOption('context_type'))) {
                $context_type = $this->io->choice('Context Type?', [
                    'server',
                    'platform',
                    'site'
                ]);
                $this->input->setOption('context_type', $context_type);
            }
            $properties = $this->askForContextProperties();
            $class = Context::getClassName($this->input->getOption('context_type'));
            $context = new $class($input->getArgument('context_name'), $this->getApplication()->getConfig()->all(), $properties);
        }

        foreach ($context->getProperties() as $name => $value) {
            $rows[] = [$name, $value];
        }
        
        $this->io->table(['Saving Context:', $context->name], $rows);
        
        if ($this->io->confirm("Write configuration for <question>{$context->type}</question> context <question>{$context->name}</question> to <question>{$context->config_path}</question>?")) {
            if ($context->save()) {
                $this->io->success("Configuration saved to {$context->config_path}");
            }
            else {
                $this->io->error("Unable to save configuration to {$context->config_path}. ");
            }
        }
        
        $output->writeln(
          "Context Object: ".print_r($context,1)
        );

//        $command = 'drush provision-save '.$input->getArgument('context_name');
//        $this->process($command);
    }

    /**
     * Loop through this context type's option_documentation() method and ask for each property.
     *
     * @return array
     */
    private function askForContextProperties() {
        $class = '\Aegir\Provision\Context\\' . ucfirst($this->input->getOption('context_type')) . "Context";
        $options = $class::option_documentation();
        $properties = [];
        foreach ($options as $name => $description) {
            $properties[$name] = $this->io->ask("$name ($description)");
        }
        return $properties;
    }
}

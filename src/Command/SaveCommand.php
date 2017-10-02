<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
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
          'server, platform, or site; default server',
          'server'
        );
        $inputDefinition[] = new InputOption(
          'delete',
          null,
          InputOption::VALUE_OPTIONAL,
          'Remove the alias.'
        );

        // @TODO: Load up all ProvisionContextTypes and inject their options.

        return new InputDefinition($inputDefinition);
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $output->writeln(
          "Saving context: ".$input->getArgument('context_name')
        );

        $command = 'drush provision-save '.$input->getArgument('context_name');
        $this->process($command);
    }
}

<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Drupal\Console\Core\Style\DrupalStyle;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class StatusCommand
 *
 * @package Aegir\Provision\Command
 */
class StatusCommand extends Command
{

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
          ->setName('status')
          ->setDescription('Display system status.')
          ->setHelp('Lists helpful information about your system.')
          ->setDefinition([
              new InputArgument(
                  'context_name',
                  InputArgument::OPTIONAL,
                  'Context to show info for.'
              )
          ])
        ;
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        
        if ($input->getArgument('context_name')) {
            $rows = [['Configuration File', $this->context->config_path]];
            foreach ($this->context->getProperties() as $name => $value) {
                if (is_string($value)) {
                    $rows[] = [$name, $value];
                }
            }
            $this->io->table(['Provision Context:', $input->getArgument('context_name')], $rows);

            // Display services.
            $this->context->showServices($this->io);
        }
        else {
            $headers = ['Provision CLI Configuration'];
            $rows = [['Configuration File', $this->getApplication()->getConfig()->getConfigPath()]];
            $config = $this->getApplication()->getConfig()->all();
            foreach ($config as $key => $value) {
                $rows[] = [$key, $value];
            }
            $this->io->table($headers, $rows);
    
            // Lookup all contexts
            $rows = [];
            foreach ($this->getApplication()->getAllContexts() as $context) {
                $rows[] = [$context->name, $context->type];
            }
            $headers = ['Contexts'];
            $this->io->table($headers, $rows);
    
            $this->io->info('Use the command `provision status CONTEXT_NAME` to show more information about that context.');
        }
    }
}

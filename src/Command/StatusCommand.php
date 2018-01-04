<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Drupal\Console\Core\Style\DrupalStyle;
use Symfony\Component\Console\Input\ArrayInput;
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
        $this->getProvision();
        
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
            $rows = [];
            $config = $this->getProvision()->getConfig()->toArray();
            unset($config['options']);
            foreach ($config as $key => $value) {
                $rows[] = [$key, $value];
            }
            $this->io->table($headers, $rows);
    
            // Lookup all contexts
            $rows = [];
            foreach ($this->getProvision()->getAllContexts() as $context) {
                $rows[] = [$context->name, $context->type];
            }
            $headers = ['Contexts'];
            if (empty($rows)) {
                $rows[] = 'There are no contexts. Run <comment>provision save</comment> to get started.';
            }
            $this->io->table($headers, $rows);
    
            // Offer to output a context status.
            $options = $this->getProvision()->getAllContextsOptions();
            if (count($options)) {
                $options['none'] = 'none';
                $context = $this->io->choiceNoList('Get status for', $options, 'none');
                if ($context != 'none') {
                    $command = $this->getApplication()->find('status');
                    $arguments['context_name'] = $context;
                    $input = new ArrayInput($arguments);
                    exit($command->run($input, $this->output));
                }
            }
        }
    }
}

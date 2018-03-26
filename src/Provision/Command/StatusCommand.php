<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
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
        ;
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $this->getProvision();
        
        if ($input->getOption('context')) {
            $rows = [['Configuration File', $this->context->config_path]];
            foreach ($this->context->getProperties() as $name => $value) {
                if (is_string($value)) {
                    $rows[] = [$name, $value];
                }
            }
            $this->io->table(['Provision Context:', $input->getOption('context')], $rows);

            // Display services.
            $this->context->showServices($this->io);
        }
        else {
            if ($this->output->isVerbose()) {
                $headers = ['Provision Console Configuration'];
                $rows = [];
                $config = $this->getProvision()->getConfig()->toArray();
                unset($config['options']);
                foreach ($config as $key => $value) {
                    $rows[] = [$key, $value];
                }
                $this->io->table($headers, $rows);
                $this->getProvision()->getLogger()->info('You can modify your console configuration using the file {path}', [
                    'path ' => $this->getProvision()->getConfig()->get('console_config_path'),
                ]);
            }

            // Lookup all contexts
            $tables = [];

            $headers['site'] = ['Sites', 'URL', 'Root'];
            $headers['server'] = ['Servers', 'services', 'Hostname'];
            $headers['platform'] = ['Platforms', 'Root'];

            foreach ($this->getProvision()->getAllContexts() as $context) {
                $method = "{$context->type}Row";
                $tables[$context->type]['rows'][] = $this->$method($context);
            }
            if (empty($tables)) {
                $this->getProvision()->io()->warningLite('There are no contexts. Run <comment>provision save</comment> to get started.');
                return;
            }

            foreach ($tables as $type => $table) {

                $this->io->table($headers[$type], $table['rows']);
            }

            // Offer to output a context status.
            $options = $this->getProvision()->getAllContextsOptions();
            if ($this->input->isInteractive() && count($options)) {
                $options['context'] = 'context';
                $context = $this->io->choiceNoList('Get status for', $options, 'select a context');
                if ($context != 'select a context') {
                    $command = $this->getApplication()->find('status');
                    $arguments['context_name'] = $context;
                    $input = new ArrayInput($arguments);
                    exit($command->run($input, $this->output));
                }
            }
        }
    }

    /**
     * Render each server row
     */
    private function serverRow(ServerContext $context) {
        return [
            $context->name,
            implode(', ', array_keys($context->getServices())),
            $context->getProperty('remote_host'),
        ];
    }
    /**
     * Render each site row
     */
    private function siteRow(SiteContext $context) {
        return [
            $context->name,
            $context->getProperty('uri'),
            $context->getProperty('root'),
        ];
    }
    /**
     * Render each platform row
     */
    private function platformRow(PlatformContext $context) {
        return [
            $context->name,
            $context->getProperty('root'),

            //            $this->sourceCell($context),
        ];
    }

    /**
     * Format git url and makefile into a single cell.
     *
     * @param \Aegir\Provision\Context $context
     *
     * @return array
     */
    private function sourceCell(Context $context, $items = []) {
        if ($context->getProperty('git_url')) {
            $items[] = $context->getProperty('git_url');
        }
        if ($context->getProperty('makefile')) {
            $items[] = $context->getProperty('makefile');
        }

        return implode("\n", $items);
    }
}

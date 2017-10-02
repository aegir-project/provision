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
          ->setHelp('Lists helpful information about your system.');
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $io = new DrupalStyle($input, $output);
        $io->comment('Provision Status');
        $headers = ['Name', 'Value'];
        $config = $this->getApplication()->getConfig()->all();
        foreach ($config as $key => $value) {
            $rows[] = [$key, $value];
        }
        $io->table($headers, $rows);

    }
}

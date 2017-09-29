<?php

namespace Aegir\Provision\Command;

use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;
use Drupal\Console\Core\Command\Command;

use Psy\Configuration;
use Psy\Shell;

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
            ->setDescription($this->trans('commands.save.description'))
            ->setHelp($this->trans('commands.save.help'));
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $config = new Configuration;
        $shell = new Shell($config);
        $shell->run();
    }
}

<?php

namespace Aegir\Provision;

use Drupal\Console\Core\Style\DrupalStyle;
use Symfony\Component\Console\Command\Command as BaseCommand;
use Drupal\Console\Core\Command\Shared\CommandTrait;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class Command
 *
 * @package Aegir\Provision\Command
 */
abstract class Command extends BaseCommand
{

    use CommandTrait;


    /**
     * @var \Symfony\Component\Console\Input\InputInterface
     */
    protected $input;

    /**
     * @var \Symfony\Component\Console\Output\OutputInterface
     */
    protected $output;
    
    /**
     * @var DrupalStyle;
     */
    protected $io;

    /**
     * @var \Aegir\Provision\Console\Config
     */
    protected $config;

    /**
     * @var \Aegir\Provision\Context;
     */
    public $context;

    /**
     * @var string
     */
    public $context_name;

    /**
     * @param InputInterface $input An InputInterface instance
     * @param OutputInterface $output An OutputInterface instance
     */
    protected function initialize(
      InputInterface $input,
      OutputInterface $output
    ) {
        $this->input = $input;
        $this->output = $output;
        
        $this->io = new DrupalStyle($input, $output);
        
        // Load active context if a command uses the argument.
        if ($this->input->hasArgument('context_name') && !empty($this->input->getArgument('context_name'))) {
            $this->context_name = $this->input->getArgument('context_name');
            $this->context = $this->getApplication()->getContext($this->context_name);
        }
    }

    /**
     * Run a process.
     *
     * @param $cmd
     */
    protected function process($cmd)
    {
        $this->output->writeln(["Running: $cmd"]);
        shell_exec($cmd);
    }

    /**
     * Gets the application instance for this command.
     *
     * @return \Aegir\Provision\Application
     *
     * @api
     */
    public function getApplication()
    {
        return parent::getApplication();
    }
}

<?php

namespace Aegir\Provision;

use Drupal\Console\Core\Style\DrupalStyle;
use Psr\Log\LogLevel;
use Symfony\Component\Console\Command\Command as BaseCommand;
use Drupal\Console\Core\Command\Shared\CommandTrait;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Logger\ConsoleLogger;
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

            try {
                // Load context from context_name argument.
                $this->context_name = $this->input->getArgument('context_name');
                $this->context = $this->getApplication()->getContext($this->context_name);
            }
            catch (\Exception $e) {

                // If no context with the specified name is found:
                // if this is "save" command and option for --delete is used, throw exception: context must exist to delete.
                if ($this->getName() == 'save' && $input->getOption('delete')) {
                    throw new \Exception("No context named {$this->context_name}. Unable to delete.");
                }
                // If this is any other command, context is required.
                elseif ($this->getName() != 'save') {
                    throw new \Exception($e->getMessage());
                }
            }
        }
        
        // If context_name is not specified, ask for it.
        elseif ($this->getDefinition()->getArgument('context_name')->isRequired() && $this->input->hasArgument('context_name') && empty($this->input->getArgument('context_name'))) {
            $this->askForContext();
            $this->input->setArgument('context_name', $this->context_name);

            try {
                $this->context = $this->getApplication()->getContext($this->context_name);
            }
            catch (\Exception $e) {
                $this->context = NULL;
            }
        }
    }
    
    /**
     * Show a list of Contexts to the user for them to choose from.
     */
    public function askForContext($question = 'Choose a context') {
        if (empty($this->getApplication()->getAllContextsOptions())) {
            $this->context_name = $this->io->ask('Context name');
        }
        else {
            $this->context_name = $this->io->choice($question, $this->getApplication()->getAllContextsOptions());
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

<?php

namespace Aegir\Provision;

use Aegir\Provision\Command\SaveCommand;
use Aegir\Provision\Command\ServicesCommand;
use Aegir\Provision\Command\ShellCommand;
use Aegir\Provision\Command\StatusCommand;
use Aegir\Provision\Command\VerifyCommand;
use Aegir\Provision\Common\ProvisionAwareTrait;
use Aegir\Provision\Console\Config;
use Aegir\Provision\Console\ConsoleOutput;
use Drupal\Console\Core\Style\DrupalStyle;
use Psr\Log\LoggerInterface;
use Psr\Log\LogLevel;
use Symfony\Component\Console\Command\HelpCommand;
use Symfony\Component\Console\Command\ListCommand;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Logger\ConsoleLogger;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Console\Application as BaseApplication;

//use Symfony\Component\DependencyInjection\ContainerInterface;
//use Drupal\Console\Annotations\DrupalCommandAnnotationReader;
//use Drupal\Console\Utils\AnnotationValidator;
//use Drupal\Console\Core\Application as BaseApplication;


/**
 * Class Application
 *
 * @package Aegir\Provision
 */
class Application extends BaseApplication
{
    /**
     * @var string
     */
    const CONSOLE_CONFIG = '.provision.yml';

    /**
     * @var string
     */
    const DEFAULT_TIMEZONE = 'America/New_York';

    use ProvisionAwareTrait;
    
    /**
     * @var ConsoleOutput
     */
    public $console;
    
    /**
     * Application constructor.
     *
     * @param \Symfony\Component\Console\Input\InputInterface   $input
     * @param \Aegir\Provision\Console\OutputInterface
     *
     * @throws \Exception
     */
    public function __construct($name = 'UNKNOWN', $version = 'UNKNOWN')
    {
        // If no timezone is set, set Default.
        if (empty(ini_get('date.timezone'))) {
            date_default_timezone_set($this::DEFAULT_TIMEZONE);
        }
//
//        // Load Configs
//        try {
//            $this->config = new Config();
//        }
//        catch (\Exception $e) {
//            throw new \Exception($e->getMessage());
//        }

        parent::__construct($name, $version);
    }
    
    /**
     * Prepare input and output arguments. Use this to extend the Application object so that $input and $output is fully populated.
     *
     * @param \Symfony\Component\Console\Input\InputInterface   $input
     * @param \Symfony\Component\Console\Output\OutputInterface $output
     */
    public function configureIO(InputInterface $input, OutputInterface $output) {
        parent::configureIO($input, $output);
        
        $this->io = new DrupalStyle($input, $output);
        
        $this->input = $input;
        $this->output = $output;
        
        $this->logger = new ConsoleLogger($output,
            [LogLevel::INFO => OutputInterface::VERBOSITY_NORMAL]
        );
    }

    /**
     * Getter for Configuration.
     *
     * @return \Aegir\Provision\Console\ProvisionConfig
     *                Configuration object.
     */
    public function getConfig()
    {
        return $this->getProvision()->getConfig();
    }

    /**
     * Initializes all the default commands.
     */
    protected function getDefaultCommands()
    {
        $commands[] = new HelpCommand();
        $commands[] = new ListCommand();
        $commands[] = new SaveCommand($this->getProvision());
        $commands[] = new ServicesCommand($this->getProvision());
//        $commands[] = new ShellCommand();
        $commands[] = new StatusCommand($this->getProvision());
        $commands[] = new VerifyCommand($this->getProvision());

        return $commands;
    }

    /**
     * {@inheritdoc}
     *
     * Adds "--target" option.
     */
    protected function getDefaultInputDefinition()
    {
        $inputDefinition = parent::getDefaultInputDefinition();
        $inputDefinition->addOption(
          new InputOption(
            '--target',
            '-t',
            InputOption::VALUE_NONE,
            'The target context to act on.'
          )
        );

        return $inputDefinition;
    }
}

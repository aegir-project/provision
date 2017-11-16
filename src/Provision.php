<?php


namespace Aegir\Provision;

use Aegir\Provision\Commands\ExampleCommands;
use Aegir\Provision\Console\Config as ConsoleConfig;

use Robo\Common\ConfigAwareTrait;
use Robo\Config\Config;
use Robo\Robo;
use Robo\Runner as RoboRunner;
use Symfony\Component\Console\Application;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

class Provision {
    
    const APPLICATION_NAME = 'Aegir Provision';
    const REPOSITORY = 'aegir-project/provision';
    
    use ConfigAwareTrait;
    
    private $runner;
    
    public function __construct(
//        Config $config,
        InputInterface $input = NULL,
        OutputInterface $output = NULL
    ) {
        
        // Create Robo configuration.
        $config = Robo::createConfiguration([ConsoleConfig::getHomeDir() . DIRECTORY_SEPARATOR . ConsoleConfig::CONFIG_FILENAME]);
        $config->setDefault('aegir_root', ConsoleConfig::getHomeDir());
        $config->setDefault('script_user', ConsoleConfig::getScriptUser());
        $config->setDefault('config_path', ConsoleConfig::getHomeDir() . '/config');
        
        $this->setConfig($config);

        // Create Application.
        $application = new \Aegir\Provision\Application(self::APPLICATION_NAME, $config->get('version'));
        $application->setConfig(new \Aegir\Provision\Console\Config());
        
        // Create and configure container.
        $container = Robo::createDefaultContainer($input, $output, $application, $config);
//        $this->setContainer($container);
//        $container->add(MyCustomService::class);
        
        // Instantiate Robo Runner.
        $this->runner = new RoboRunner([
            ExampleCommands::class
        ]);
        
        $this->runner->setContainer($container);
        $this->runner->setSelfUpdateRepository(self::REPOSITORY);
    }
    
    public function run(InputInterface $input, OutputInterface $output) {
        $status_code = $this->runner->run($input, $output);
        
        return $status_code;
    }
    
}

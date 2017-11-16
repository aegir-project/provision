<?php


namespace Aegir\Provision;

use Aegir\Provision\Commands\ExampleCommands;
use Aegir\Provision\Console\Config as ConsoleConfig;

use Consolidation\Config\Loader\ConfigProcessor;
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
        
        // Prepare Console configuration and import it into Robo config.
        $consoleConfig = new \Aegir\Provision\Console\Config();

        $config = new Config();
        $config->import($consoleConfig->all());
        
        $this->setConfig($config);

        // Create Application.
        $application = new \Aegir\Provision\Application(self::APPLICATION_NAME, $config->get('version'));
        $application->setConfig($consoleConfig);
        
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

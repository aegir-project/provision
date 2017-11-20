<?php


namespace Aegir\Provision;

use Aegir\Provision\Console\Config;
use Aegir\Provision\Commands\ExampleCommands;

use Aegir\Provision\Robo\ProvisionCollectionBuilder;
use Aegir\Provision\Robo\ProvisionExecutor;
use Aegir\Provision\Robo\ProvisionTasks;
use League\Container\Container;
use League\Container\ContainerAwareInterface;
use League\Container\ContainerAwareTrait;
use Psr\Log\LoggerAwareInterface;
use Psr\Log\LoggerAwareTrait;
use Robo\Collection\CollectionBuilder;
use Robo\Common\BuilderAwareTrait;
use Robo\Common\ConfigAwareTrait;
use Robo\Common\IO;
use Robo\Contract\BuilderAwareInterface;
use Robo\Contract\ConfigAwareInterface;
use Robo\Contract\IOAwareInterface;
use Robo\Log\RoboLogger;
use Robo\Robo;
use Robo\Runner as RoboRunner;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class Provision
 *
 * Uses BuilderAwareTrait to allow access to Robo Tasks:
 *
 * $this->getBuilder()->taskExec('ls -la')
 *   ->run()
 *
 * @package Aegir\Provision
 */
class Provision implements ConfigAwareInterface, ContainerAwareInterface, LoggerAwareInterface, IOAwareInterface, BuilderAwareInterface {
    
    const APPLICATION_NAME = 'Aegir Provision';
    const VERSION = '4.x-dev';
    const REPOSITORY = 'aegir-project/provision';
    
    use BuilderAwareTrait;
    use ConfigAwareTrait;
    use ContainerAwareTrait;
    use LoggerAwareTrait;
    use IO;
    
    /**
     * @var \Robo\Runner
     */
    private $runner;
    
    /**
     * @var string[]
     */
    private $commands = [];
    
    /**
     * Provision constructor.
     *
     * @param \Aegir\Provision\Console\Config                        $config
     * @param \Symfony\Component\Console\Input\InputInterface|null   $input
     * @param \Symfony\Component\Console\Output\OutputInterface|null $output
     */
    public function __construct(
        Config $config,
        InputInterface $input = NULL,
        OutputInterface $output = NULL
    ) {
        $this->setConfig($config);
        $this->setInput($input);
        $this->setOutput($output);
        
        // Create Application.
        $application = new Application(self::APPLICATION_NAME, self::VERSION);
        $application->setProvision($this);
        
//        $application->setConfig($consoleConfig);
        
        // Create and configure container.
        $container = Robo::createDefaultContainer($input, $output, $application, $config);
        $this->setContainer($container);
        $this->configureContainer($container);
        
        // Instantiate Robo Runner.
        $this->runner = new RoboRunner([
            ExampleCommands::class
        ]);
        
        $this->runner->setContainer($container);
        $this->runner->setSelfUpdateRepository(self::REPOSITORY);
    
        $this->setBuilder($container->get('builder'));
        $this->setLogger($container->get('logger'));
    }
    
    public function run(InputInterface $input, OutputInterface $output) {
        $status_code = $this->runner->run($input, $output);
        
        return $status_code;
    }
    
    /**
     * Register the necessary classes for Provision.
     */
    public function configureContainer(Container $container) {
    
        // FROM https://github.com/acquia/blt :
        // We create our own builder so that non-command classes are able to
        // implement task methods, like taskExec(). Yes, there are now two builders
        // in the container. "collectionBuilder" used for the actual command that
        // was executed, and "builder" to be used with non-command classes.
        $tasks = new ProvisionTasks();
        $builder = new ProvisionCollectionBuilder($tasks);
        $tasks->setBuilder($builder);
        $container->add('builder', $builder);
        $container->add('executor', ProvisionExecutor::class)
            ->withArgument('builder');
    }
    
    /**
     * Temporary helper to allow public access to output.
     *
     * @return \Symfony\Component\Console\Output\OutputInterface
     */
    public function getOutput()
    {
        return $this->output();
    }
    
    /**
     * Temporary helper to allow public access to input.
     *
     * @return \Symfony\Component\Console\Input\InputInterface
     */
    public function getInput()
    {
        return $this->input();
    }
    
    
    /**
     * Load all contexts into Context objects.
     *
     * @return array
     */
    static function getAllContexts($name = '', $application = NULL) {
        $contexts = [];
        $config = new Config();
        
        $context_files = [];
        $finder = new \Symfony\Component\Finder\Finder();
        $finder->files()->name('*' . $name . '.yml')->in($config->get('config_path') . '/provision');
        foreach ($finder as $file) {
            list($context_type, $context_name) = explode('.', $file->getFilename());
            $context_files[$context_name] = [
                'name' => $context_name,
                'type' => $context_type,
                'file' => $file,
            ];
        }
        
        foreach ($context_files as $context) {
            $class = Context::getClassName($context['type']);
            $contexts[$context['name']] = new $class($context['name'], $application);
        }
        
        if ($name && isset($contexts[$name])) {
            return $contexts[$name];
        }
        elseif ($name && !isset($contexts[$name])) {
            return NULL;
        }
        else {
            return $contexts;
        }
    }
    
    /**
     * Load all server contexts.
     *
     * @param null $service
     * @return mixed
     * @throws \Exception
     */
    static public function getAllServers($service = NULL) {
        $servers = [];
        $context_files = self::getAllContexts();
        if (empty($context_files)) {
            throw new \Exception('No server contexts found. Use `provision save` to create one.');
        }
        foreach ($context_files as $context) {
            if ($context->type == 'server') {
                $servers[$context->name] = $context;
            }
        }
        return $servers;
    }
    
    /**
     * Get a simple array of all contexts, for use in an options list.
     * @return array
     */
    public function getAllContextsOptions($type = NULL) {
        $options = [];
        foreach ($this->getAllContexts() as $name => $context) {
            if ($type) {
                if ($context->type == $type) {
                    $options[$name] = $context->name;
                }
            }
            else {
                $options[$name] = $context->type . ' ' . $context->name;
            }
        }
        return $options;
    }
    
    /**
     * Load the Aegir context with the specified name.
     *
     * @param $name
     *
     * @return \Aegir\Provision\Context
     * @throws \Exception
     */
    static public function getContext($name, Application $application = NULL) {
        if (empty($name)) {
            throw new \Exception('Context name must not be empty.');
        }
        if (empty(Provision::getAllContexts($name, $application))) {
            throw new \Exception('Context not found with name: ' . $name);
        }
        return Provision::getAllContexts($name, $application);
    }
    
    /**
     * Get a simple array of all servers, optionally specifying the the service_type to filter by ("http", "db" etc.)
     * @param string $service_type
     * @return array
     */
    public function getServerOptions($service_type = '') {
        $servers = [];
        foreach ($this->getAllServers() as $server) {
            if ($service_type && !empty($server->config['services'][$service_type])) {
                $servers[$server->name] = $server->name . ': ' . $server->config['services'][$service_type]['type'];
            }
            elseif ($service_type == '') {
                $servers[$server->name] = $server->name . ': ' . $server->config['services'][$service_type]['type'];
            }
        }
        return $servers;
    }
    
    /**
     * Check that a context type's service requirements are provided by at least 1 server.
     *
     * @param $type
     * @return array
     */
    static function checkServiceRequirements($type) {
        $class_name = Context::getClassName($type);
        
        // @var $context Context
        $service_requirements = $class_name::serviceRequirements();
        
        $services = [];
        foreach ($service_requirements as $service) {
            try {
                if (empty(Provision::getAllServers($service))) {
                    $services[$service] = 0;
                }
                else {
                    $services[$service] = 1;
                }
            } catch (\Exception $e) {
                $services[$service] = 0;
            }
        }
        return $services;
    }
}

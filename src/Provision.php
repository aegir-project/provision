<?php


namespace Aegir\Provision;

use Aegir\Provision\Console\Config;
use Aegir\Provision\Commands\ExampleCommands;

use Aegir\Provision\Console\ConsoleOutput;
use Aegir\Provision\Robo\ProvisionCollectionBuilder;
use Aegir\Provision\Robo\ProvisionExecutor;
use Aegir\Provision\Robo\ProvisionTasks;
use Drupal\Console\Core\Style\DrupalStyle;
use League\Container\Container;
use League\Container\ContainerAwareInterface;
use League\Container\ContainerAwareTrait;
use Psr\Log\LoggerAwareInterface;
use Psr\Log\LoggerAwareTrait;
use Psr\Log\LogLevel;
use Robo\Common\BuilderAwareTrait;
use Robo\Common\ConfigAwareTrait;
use Robo\Common\IO;
use Robo\Contract\BuilderAwareInterface;
use Robo\Contract\ConfigAwareInterface;
use Robo\Contract\IOAwareInterface;
use Robo\LoadAllTasks;
use Robo\Robo;
use Robo\Runner as RoboRunner;
use Symfony\Component\Console\Input\ArgvInput;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Logger\ConsoleLogger;
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
    
    const APPLICATION_NAME = 'Provision';
    const APPLICATION_FUN_NAME = '𝙋𝙍𝙊𝙑𝙄𝙎𝙄𝙊𝙉';
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
     * @var ProvisionTasks
     */
    private $tasks;
    
    /**
     * @var string[]
     */
    private $commands = [];
    
    /**
     * @var \Aegir\Provision\Application
     */
    private $application;
    
    /**
     * @var \Aegir\Provision\Context[]
     */
    private $contexts = [];
    
    /**
     * @var array[]
     */
    private $context_files = [];
    
    /**
     * @var ConsoleOutput
     */
    public $console;
    
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
    
        $logger = new ConsoleLogger($output);
        $this->setLogger($logger);
        
        $this
            ->setConfig($config)
        ;

        // Create Application.
        $application = new Application(self::APPLICATION_NAME, self::VERSION);
        $application
            ->setProvision($this)
            ->setLogger($logger)
        ;
        $application->configureIO($input, $output);
        $this->setInput($input);
        $this->setOutput($output);

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
        
        $this->tasks = $container->get('tasks');
        $this->console = new ConsoleOutput($output->getVerbosity());
        $this->console->setProvision($this);
        
        $this->loadAllContexts();
    }
    
    /**
     * Lookup all context yml files and load into Context classes.
     */
    private function loadAllContexts()
    {
        $folder = $this->getConfig()->get('config_path') . '/provision';
        $finder = new \Symfony\Component\Finder\Finder();
        $finder->files()->name("*.yml")->in($folder);
        foreach ($finder as $file) {
            $context_type = substr($file->getFilename(), 0, strpos($file->getFilename(), '.'));
            $context_name = substr($file->getFilename(), strpos($file->getFilename(), '.') + 1, strlen($file->getFilename()) - strlen($context_type) - 5);
        
            $this->context_files[$context_name] = [
                'name' => $context_name,
                'type' => $context_type,
                'file' => $file,
            ];
        }

        // Load Context classes from files metadata.
        foreach ($this->context_files as $context) {
            $class = Context::getClassName($context['type']);
            $this->contexts[$context['name']] = new $class($context['name'], $this);
        }
    }
    
    /**
     * Loads a single context from file into $this->contexts[$name].
     *
     * Used to load dependant contexts that might not be instantiated yet.
     *
     * @param $name
     */
    public function loadContext($name) {
        $class = Context::getClassName($this->context_files[$name]['type']);
        $this->contexts[$this->context_files[$name]['name']] = new $class($this->context_files[$name]['name'], $this);
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
        $tasks->setLogger($this->logger);
        $builder = new ProvisionCollectionBuilder($tasks);
        $builder->setProvision($this);
        $tasks->setBuilder($builder);
        $container->add('tasks', $tasks);
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
     * Gets Application object.
     *
     * @return \Aegir\Provision\Application
     */
    public function getApplication()
    {
        return $this->application;
    }
    /**
     * Gets Logger object.
     * Returns the currently active Logger instance.
     *
     * @return \Psr\Log\LoggerInterface
     */
    public function getLogger()
    {
        return $this->logger;
    }
    
    /**
     * @return ProvisionTasks
     */
    public function getTasks()
    {
        return $this->tasks;
    }
    
    /**
     * Provide access to DrupalStyle object.
     *
     * @return \Drupal\Console\Core\Style\DrupalStyle
     */
    public function io()
    {
        if (!$this->io) {
            $this->io = new DrupalStyle($this->input(), $this->output());
        }
        return $this->io;
    }
    
    /**
     * Get a new Provision
     * @return \Aegir\Provision\Provision
     */
    static function getProvision() {
        $input = new ArgvInput();
        $output = new ConsoleOutput();
        $config = new Config();
        return new Provision($config, $input, $output);
    }
    
    /**
     * Return all available contexts.
     *
     * @return array|Context
     */
    public function getAllContexts($name = '') {
        if ($name && isset($this->contexts[$name])) {
            return $this->contexts[$name];
        }
        elseif ($name && !isset($this->contexts[$name])) {
            return NULL;
        }
        else {
            return $this->contexts;
        }
    }
    
    /**
     * Load all server contexts.
     *
     * @param null $service
     * @return mixed
     * @throws \Exception
     */
    protected function getAllServers($service = NULL) {
        $servers = [];
        $context_files = $this->getAllContexts();
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
     * @return array|\Aegir\Provision\Context
     * @throws \Exception
     */
    public function getContext($name) {
        // Check if $name is empty.
        if (empty($name)) {
            throw new \Exception('Context name must not be empty.');
        }
        
        // If context exists but hasn't been loaded, load it.
        if (empty($this->contexts[$name]) && !empty($this->context_files[$name])) {
            $this->loadContext($name);
        }
        
        // Check if context still isn't found.
        if (empty($this->contexts[$name])) {
            throw new \Exception('Context not found with name: ' . $name);
        }
        return $this->contexts[$name];
    }
    
    /**
     * Look for a context file being present. This is available before Context
     * objects are bootstrapped.
     */
    public function getContextFile($name) {
        if (empty($name)) {
            throw new \Exception('Context name must not be empty.');
        }
        if (empty($this->context_files[$name])) {
            throw new \Exception('Context not found with name: ' . $name);
        }
        return$this->context_files[$name];
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
    public function checkServiceRequirements($type) {
        $class_name = Context::getClassName($type);
        
        // @var $context Context
        $service_requirements = $class_name::serviceRequirements();
        
        $services = [];
        foreach ($service_requirements as $service) {
            try {
                if (empty($this->getAllServers($service))) {
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
    
    /**
     * Determine the user running provision.
     */
    public function getScriptUid() {
        return posix_getuid();
    }

    static public function newTask() {
        return new Task();
    }
    static public function newProperty($description = '') {
        return new Property($description);
    }
}

<?php
/**
 * @file
 * Provides the Aegir\Provision\Context class.
 */

namespace Aegir\Provision;

use Aegir\Provision\Common\ProvisionAwareTrait;
use Aegir\Provision\Console\Config;
use Aegir\Provision\Console\ProvisionStyle;
use Consolidation\AnnotatedCommand\CommandFileDiscovery;
use Drupal\Console\Core\Style\DrupalStyle;
use Robo\Collection\CollectionBuilder;
use Robo\Common\BuilderAwareTrait;
use Robo\Common\ProgressIndicator;
use Robo\Contract\BuilderAwareInterface;
use Robo\ResultData;
use Robo\Tasks;
use Symfony\Component\Config\Definition\Builder\TreeBuilder;
use Symfony\Component\Config\Definition\Processor;
use Symfony\Component\Console\Exception\InvalidOptionException;
use Symfony\Component\Console\Exception\RuntimeException;
use Symfony\Component\Console\Helper\ProgressBar;
use Symfony\Component\Filesystem\Exception\IOException;
use Symfony\Component\Filesystem\Filesystem;
use Symfony\Component\Process\Process;
use Symfony\Component\Yaml\Dumper;
use Symfony\Component\Yaml\Yaml;

/**
 * Class Context
 *
 * Base context class.
 *
 * @package Aegir\Provision
 */
class Context implements BuilderAwareInterface
{

    use BuilderAwareTrait;
    use ProvisionAwareTrait;

    /**
     * @var string
     * Name for saving aliases and referencing.
     */
    public $name = null;

    /**
     * @var string
     * 'server', 'platform', or 'site'.
     */
    public $type = null;
    const TYPE = null;

    /**
     * The role of this context, either 'subscriber' or 'provider'.
     *
     * @var string
     */
    const ROLE = null;

    /**
     * @var string
     * Full path to this context's config file.
     */
    public $config_path = null;

    /**
     * @var array
     * Properties that will be persisted by provision-save. Access as object
     * members, $evironment->property_name. __get() and __set handle this. In
     * init(), set defaults with setProperty().
     */
    protected $properties = [];

    /**
     * @var array
     * A list of services provided by or subscribed to by this context.
     */
    protected $services = [];

    /**
     * @var \Symfony\Component\Filesystem\Filesystem
     */
    public $fs;

    /**
     * Context constructor.
     *
     * @param $name
     * @param array $options
     */
    function __construct(
        $name,
        Provision $provision,
        $options = [])
    {
        $this->name = $name;

        $this->setProvision($provision);
        $this->setBuilder($this->getProvision()->getBuilder());

        $this->loadContextConfig($options);
        $this->prepareServices();

        $this->fs = new Filesystem();
    }

    /**
     * Load and process the Config object for this context.
     *
     * @param array $options
     *
     * @throws \Exception
     */
    private function loadContextConfig($options = []) {

        if ($this->getProvision()) {
            $this->config_path = $this->getProvision()->getConfig()->get('contexts_path') . DIRECTORY_SEPARATOR . $this->type . '.' . $this->name . '.yml';
        }
        else {
            $config = new Config();
            $this->config_path = $config->get('contexts_path') . DIRECTORY_SEPARATOR . $this->type . '.' . $this->name . '.yml';
        }

        $configs = [];

        try {
            $processor = new Processor();
            if (file_exists($this->config_path)) {
                $this->properties = Yaml::parse(file_get_contents($this->config_path));
            }
            else {
                // Load command line options into properties
                foreach ($this->option_documentation() as $option => $description) {
                    if (isset($options[$option])) {
                        $this->properties[$option] = $options[$option];
                    }
                    else {
                        $this->properties[$option] = NULL;
                    }
                }
            }

            $this->properties['type'] = $this->type;
            $this->properties['name'] = $this->name;

            $configs[] = $this->properties;

            $this->config = $processor->processConfiguration($this, $configs);

        } catch (\Exception $e) {
            throw new InvalidOptionException(
                strtr("There is an error with the configuration for !type '!name'. Check the file !file and try again. \n \nError: !message", [
                    '!type' => $this->type,
                    '!name' => $this->name,
                    '!message' => $e->getMessage(),
                    '!file' => $this->config_path,
                ])
            );
        }
    }

    /**
     * Load Service classes from config into Context services or serviceSubscriptions.
     */
    protected function prepareServices() {}

    /**
     * Loads all available \Aegir\Provision\Service classes
     *
     * @return array
     */
    static public function findAvailableServices($service = '') {

        // Load all service classes
        $classes = [];
        $discovery = new CommandFileDiscovery();
        $discovery->setSearchPattern('*Service.php');
        $servicesFiles = $discovery->discover(__DIR__ .'/Service', '\Aegir\Provision\Service');
        foreach ($servicesFiles as $serviceClass) {
            // If this is a server, show all services. If it is not, but service allows this type of context, load it.
//            if (self::TYPE == 'server' || in_array(self::TYPE, $serviceClass::allowedContexts())) {
                $classes[$serviceClass::SERVICE] = $serviceClass;
//            }
        }
        return $classes;
//
//        if ($service && isset($classes[$service])) {
//            return $classes[$service];
//        }
//        elseif ($service && !isset($classes[$service])) {
//            throw new \Exception("No service with name $service was found.");
//        }
//        else {
//            return $classes;
//        }
    }

    /**
     * @param string $service
     */
    public function getAvailableServices($service = '')
    {
        $services_return = [];
        $services = $this->findAvailableServices();
        foreach ($services as $serviceClass) {
            // If this is a server, show all services. If it is not, but service allows this type of context, load it.
            if ($this::TYPE == 'server' || in_array($this::TYPE, $serviceClass::allowedContexts())) {
                $services_return[$serviceClass::SERVICE] = $serviceClass;
            }
        }

        if ($service && isset($services_return[$service])) {
            return $services_return[$service];
        }
        elseif ($service && !isset($services_return[$service])) {
            throw new \Exception("No service with name $service was found.");
        }
        else {
            return $services_return;
        }
    }
    /**
     * Lists all available services as a simple service => name array.
     * @return array
     */
    static public function getServiceOptions() {
        $options = [];
        $services = self::findAvailableServices();
        foreach ($services as $service => $class) {
            $options[$service] = $class::SERVICE_NAME;
        }
        return $options;
    }

    /**
     * @return array
     */
    static function getAvailableServiceTypes($service, $service_type = NULL) {

        // Load all service classes
        $classes = [];
        $discovery = new CommandFileDiscovery();
        $discovery->setSearchPattern(ucfirst($service) . '*Service.php');
        $serviceTypesFiles = $discovery->discover(__DIR__ .'/Service/' . ucfirst($service), '\Aegir\Provision\Service\\' . ucfirst($service));
        foreach ($serviceTypesFiles as $serviceTypeClass) {
            $classes[$serviceTypeClass::SERVICE_TYPE] = $serviceTypeClass;
        }

        if ($service_type && isset($classes[$service_type])) {
            return $classes[$service_type];
        }
        elseif ($service_type && !isset($classes[$service_type])) {
            throw new \Exception("No service type with name $service_type was found.");
        }
        else {
            return $classes;
        }
    }

    /**
     * Lists all available services as a simple service => name array.
     * @return array
     */
    static public function getServiceTypeOptions($service) {
        $options = [];
        $service_types = self::getAvailableServiceTypes($service);
        foreach ($service_types as $service_type => $class) {
            $options[$service_type] = $class::SERVICE_TYPE_NAME;
        }
        return $options;
    }

    /**
     * Lists all available context types as a simple service => name array.
     *
     * @TODO: Make this dynamically load from classes, like getServiceTypeOptions()
     * @return array
     */
    static public function getContextTypeOptions() {
        return [
            'server' => 'Server',
            'platform' => 'Platform',
            'site' => 'Site',
        ];
    }


    /**
     * Return a specific service provided by this context.
     *
     * @param $type
     *
     * @return \Aegir\Provision\Service
     */
    public function getService($type) {
        if (isset($this->services[$type])) {
            return $this->services[$type];
        }
        else {
            throw new \Exception("Service '$type' does not exist in the context '{$this->name}'.");
        }
    }

    /**
     * Whether or not this Server has a service.
     *
     * @param $type
     * @return bool
     */
    public function hasService($type) {
        if (isset($this->services[$type])) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    /**
     * Return all services this context provides.
     *
     * @return array
     */
    public function getServices() {
        return $this->services;
    }

    /**
     * Return all services for this context.
     *
     * @return \Aegir\Provision\Service
     */
    public function service($type)
    {
        return $this->getService($type);
    }

    /**
     * Call method $callback on each of the context's service objects.
     *
     * @param $callback
     *   A Provision_Service method.
     * @return
     *   An array of return values from method implementations.
     */
    function servicesInvoke($callback, array $args = array()) {
      $results = array();
      // fetch the merged list of services.
      // These may be on different servers entirely.
      $services = $this->getServices();
      foreach ($services as $service_name => $service) {
        if (method_exists($service, $callback)) {
          $results[$service_name] = call_user_func_array(array($service, $callback), $args);
        }
      }
      return $results;
    }

  /**
     * {@inheritdoc}
     */
    public function getConfigTreeBuilder()
    {
        $tree_builder = new TreeBuilder();
        $root_node = $tree_builder->root($this->type);
        $root_node
            ->children()
                ->scalarNode('name')
                    ->defaultValue($this->name)
                    ->isRequired()
                ->end()
                ->scalarNode('type')
                    ->defaultValue($this->type)
                    ->isRequired()
                ->end()
            ->end();

        // Load Services.
        $this->servicesConfigTree($root_node);

        // @TODO: Figure out how we can let other classes add to Context properties.
        foreach ($this->option_documentation() as $name => $description) {
            $this->properties[$name] = empty($this->properties[$name])? '': $this->properties[$name];
            $root_node
                ->children()
                    ->scalarNode($name)
                        ->defaultValue($this->properties[$name])
                    ->end()
                ->end();
        }

        // Load contextRequirements into config as ContextNodes.
        foreach ($this->contextRequirements() as $property => $type) {
            $root_node
                ->children()
                    ->setNodeClass('context', 'Aegir\Provision\Console\ConfigContextNodeDefinition')
                    ->node($property, 'context')
                        ->isRequired()
                        ->attribute('context_type', $type)
                        ->attribute('provision', $this->getProvision())
                    ->end()
                ->end();
        }

        if (method_exists($this, 'configTreeBuilder')) {
            $this->configTreeBuilder($root_node);
        }

        return $tree_builder;
    }

    /**
     * Prepare either services or service subscriptions config tree.
     */
    protected function servicesConfigTree(&$root_node) {}

    /**
     * Append Service class options_documentation to config tree.
     */
    public function addServiceProperties($property_name = 'services')
    {
        $builder = new TreeBuilder();
        $node = $builder->root('properties');

        // Load config tree from Service type classes
        if (!empty($this->getProperty($property_name)) && !empty($this->getProperty($property_name))) {
            foreach ($this->getProperty($property_name) as $service => $info) {

                // If type is empty, it's because it's in the ServerContext
                if (empty($info['type'])) {
                    $server = $this->getProvision()->getContext($info['server']);
                    $service_type = ucfirst($server->getService($service)->type);
                }
                else {
                    $service_type = ucfirst($info['type']);
                }
                $class = Service::getClassName($service, $service_type);
                $method = "{$this->type}_options";

                foreach ($class::{$method}() as $name => $description) {
                    $node
                        ->children()
                        ->scalarNode($name)->end()
                        ->end()
                        ->end();
                }
            }
        }
        return $node;
    }

    /**
     * Output a list of all services for this context.
     */
    public function showServices(DrupalStyle $io) {

        $services = $this->isProvider()? $this->getServices(): $this->getSubscriptions();
        if (!empty($services)) {
            $rows = [];

            $headers = $this->isProvider()?
                ['Services']:
                ['Service', 'Server', 'Type'];

            foreach ($services as $name => $service) {
                if ($this::ROLE == 'provider') {
                    $rows[] = [$name, $service->type];
                }
                else {
                    $rows[] = [
                        $name,
                        $service->server->name,
                        $service->server->getService($name)->type
                    ];
                }

                // Show all properties.
                if (!empty($service->properties )) {
                    foreach ($service->properties as $name => $value) {
                        $rows[] = ['  ' . $name, $value];
                    }
                }
            }
            $io->table($headers, $rows);
        }
    }

    /**
     * Return all properties for this context.
     *
     * @return array
     */
    public function getProperties() {
        return $this->properties;
    }

    /**
     * Return all properties for this context.
     *
     * @return array
     */
    public function getProperty($name) {
        if (isset($this->properties[$name])) {
            return $this->properties[$name];
        }
        else {
          return NULL;
        }
    }

    /**
     * Set a specific property.
     *
     * @param $name
     * @return mixed
     * @throws \Exception
     */
    public function setProperty($name, $value) {
        $this->properties[$name] = $value;
    }

    /**
     * Subclasses can implement to modify properties before saving.
     */
    public function preSave() {}

    /**
     * Saves the config class to file.
     *
     * @return bool
     */
    public function save()
    {

        // Create config folder if it does not exist.
        $fs = new Filesystem();
        $dumper = new Dumper();

        try {
            $fs->dumpFile($this->config_path, $dumper->dump($this->getProperties(), 10));
            return true;
        } catch (IOException $e) {
            return false;
        }
    }

    /**
     * Deletes the config YML file.
     * @return bool
     */
    public function deleteConfig() {

        // Create config folder if it does not exist.
        $fs = new Filesystem();

        try {
            $fs->remove($this->config_path);
            return true;
        } catch (IOException $e) {
            return false;
        }
    }

    /**
     * Retrieve the class name of a specific context type.
     *
     * @param $type
     *   The type of context, typically server, platform, site.
     *
     * @return string
     *   The fully-qualified class name for this type.
     */
    static function getClassName($type) {
        return '\Aegir\Provision\Context\\' . ucfirst($type) . "Context";
    }

//    public function verify() {
//        return "Provision Context";
//    }

    /**
     * Verify this context.
     *
     * Running `provision verify CONTEXT` triggers this method.
     *
     * Collect all services for the context and run the verify() method on them.
     *
     * If this context is a Service Subscriber, the provider service will be verified first.
     */
    public function verifyCommand()
    {
        $collection = $this->getProvision()->getBuilder();

        $pre_tasks = $this->preVerify();
        $pre_tasks += $this->verify();
        foreach ($pre_tasks as $pre_task_title => $pre_task) {
            $collection->getConfig()->set($pre_task_title, $pre_task);
            $this->prepareTask($collection, $pre_task_title, $pre_task);
        }
        foreach ($this->getServices() as $type => $service) {
            $friendlyName = $service->getFriendlyName();
            $tasks = [];
//
//            $collection->addCode(function() use ($friendlyName, $type) {
//                $this->getProvision()->io()->section("Verify service: {$friendlyName}");
//            }, 'logging.' . $type);
            $tasks['logging.' . $type] = function() use ($friendlyName, $type) {
                $this->getProvision()->io()->section("Verify service: {$friendlyName}");
            };

            $service->setContext($this);
            $tasks = array_merge($tasks, $service->verify());

            foreach ($tasks as $title => $task) {
                $this->prepareTask($collection, $title, $task, $service);
            }
            $tasks = [];
        }

        $result = $collection->run();

        if ($result->wasSuccessful()) {
            $this->getProvision()->io()->success('Verification Complete!');
        }
        else {
            throw new RuntimeException('Some services did not verify. Check your configuration, or run with the verbose option (-v) for more information.');
        }
    }

    /**
     * Helper to pass tasks to the collection.
     * @param $collection
     * @param $title
     * @param $task
     * @param $service
     * @throws \Exception
     */
    function prepareTask($collection, $title, $task, $service = NULL) {
        if (is_callable($task)) {
            $task = Provision::newTask()
              ->execute($task);
        }

        $collection->getConfig()->set($title, $task);

        if ($task instanceof \Robo\Task || $task instanceof \Robo\Collection\CollectionBuilder) {
            $collection->getCollection()->add($task, $title);
        }
        elseif ($task instanceof Task) {
            $collection->addCode($task->callable, $title);
        }
        else {
            $class = get_class($task);
            if ($service) {
                $friendlyName = $service->getFriendlyName();
                throw new \Exception("Task '$title' in service '$friendlyName' must be a callable or \\Robo\\Collection\\CollectionBuilder. Is class '$class'");
            }
            else {
                throw new \Exception("Task '$title' must be a callable or \\Robo\\Collection\\CollectionBuilder. Is class '$class'");
            }
        }
    }


    /**
     * Stub to be implemented by context types. Return tasks to be run before
     * any other verify takes place.
     */
    function preVerify() {
      $tasks = [];

      // Lookup possible hook files.
      // @TODO: create methods on the contexts to do this.
      if ($this->type == 'server') {
        $lookup_paths[] = $this->server_config_path;
      }
      elseif ($this->type == 'site' || $this->type == 'platform') {
        $lookup_paths[] = $this->getProperty('root');
      }

      foreach ($lookup_paths as $i => $path) {
        $yml_file_path = $path . '/.provision.yml';
        $yml_file_path_machine_name = preg_replace('/[^a-zA-Z0-9\']/', '_', $yml_file_path);

        if (file_exists($yml_file_path)) {
          $tasks['yml_hooks_found'] = Provision::newTask()
            ->start("Custom hooks file found: <comment>{$yml_file_path}</comment>")
            ->failure("Custom hooks file found: <comment>{$yml_file_path}</comment>: Unable to parse YAML.")
            ->execute(function () use ($yml_file_path) {
              // Attempt to parse Yml here so we alert the user early.
              $this->custom_yml_data = Yaml::parseFile($yml_file_path);
              })
          ;

          $tasks['yml_hooks_' . $yml_file_path_machine_name] = Provision::newTask()
            ->start("Running <comment>hooks:verify:pre</comment> from {$yml_file_path}")
            ->success("Successfully ran <comment>hooks:verify:pre</comment> from {$yml_file_path}")
            ->failure("Errors while running <comment>hooks:verify:pre</comment> from {$yml_file_path}")
            ->startPrefix("<fg=blue>" . ProvisionStyle::ICON_BULLET . "</>")
            ->execute(function () use ($yml_file_path){
              $this->getProvision()->getOutput()->writeln([]);
              $this->getProvision()->getOutput()->writeln([]);
              $this->getProvision()->getOutput()->writeln([]);

              // @TODO: Put this in it's own method, it probably needs a whole class, really.
              if (isset($this->custom_yml_data['hooks']['verify']['pre'])) {
                $command = 'set -e; ' . $this->custom_yml_data['hooks']['verify']['pre'];
                return $this->process_exec($command);
              }
            })
          ;
        }
      }
       return $tasks;
    }

    /**
     * Stub to be implemented by context types.
     *
     * Run extra tasks before services take over.
     */
    function verify() {
       return [];
    }
//
//
//        $return_codes = [];
//        // Run verify method on all services.
//        foreach ($this->getServices() as $type => $service) {
//            $friendlyName = $service->getFriendlyName();
//
//            if ($this->isProvider()) {
//                $this->getProvision()->io()->section("Verify service: {$friendlyName}");
//
//                // @TODO: Make every service use collections
//                $this->getProvision()->getLogger()->info('Verify service: ' . get_class($service));
//                $verify = $service->verify();
//                if ($verify instanceof CollectionBuilder) {
////                    $this->getProvision()->console->runCollection($verify);
//
//                    $collection->addIterable($verify);
//
//                    $result = $verify->run();
//                    $return_codes[] = $result->wasSuccessful()? 0: 1;
//                }
//                // @TODO: Remove this once all services use CollectionBuilders.
//                elseif (is_array($verify)) {
//                    foreach ($service->verify() as $type => $verify_success) {
//                        $return_codes[] = $verify_success? 0: 1;
//                    }
//                }
//            }
//            else {
//                $this->getProvision()->io()->section("Verify service: {$friendlyName} on {$service->provider->name}");
//
//                // First verify the service provider.
//                foreach ($service->verifyProvider() as $verify_part => $verify_success) {
//                    $return_codes[] = $verify_success? 0: 1;
//                }
//
//                // Then run "verify" on the subscriptions.
//                foreach ($this->getSubscription($type)->verify() as $type => $verify_success) {
//                    $return_codes[] = $verify_success? 0: 1;
//                }
//            }
//        }
//
//        // If any service verify failed, exit with a non-zero code.
//        if (count(array_filter($return_codes))) {
//            throw new \Exception('Some services did not verify. Check your configuration and try again.');
//        }
//    }

    /**
     * Return an array of required services for this context.
     * Example:
     *   return ['http'];
     */
    public static function serviceRequirements() {
        return [];
    }

    /**
     * Return an array of required contexts in the format PROPERTY_NAME => CONTEXT_TYPE.
     *
     * Example:
     *   return ['platform' => 'platform'];
     */
    public static function contextRequirements() {
        return [];
    }

    /**
     * Whether or not this context is a provider.
     *
     * @return bool
     */
    public function isProvider(){
        return $this::ROLE == 'provider';
    }

    /**
     * Whether or not this context is a provider.
     *
     * @return bool
     */
    public function isSubscriber(){
        return $this::ROLE == 'subscriber';
    }

//    /**
//     * Check that there is at least one server that provides for each serviceRequirements().
//     *
//     * @return array
//     *   The key is the service, the value is 0 if not available or 1 if is available.
//     */
//    function checkRequirements() {
//        $reqs = self::serviceRequirements();
//        $services = [];
//        foreach ($reqs as $service) {
//            if (empty($this->application->getAllServers($service))) {
//                $services[$service] = 0;
//            }
//            else {
//                $services[$service] = 1;
//            }
//        }
//        return $services;
//    }

  /**
   * Run a shell command on this server.
   *
   * @param $cmd string The command to run
   * @param $dir string The directory to run the command in. Defaults to this server's config path.
   * @param $return string What to return. Can be 'output' or 'exit'.
   *
   * @return string
   * @throws \Exception
   */
  public function shell_exec($command, $dir = NULL, $return = 'stdout') {
    $cwd = getcwd();
    $original_command = $command;

    $tmpdir = sys_get_temp_dir() . '/provision';
    if (!$this->fs->exists($tmpdir)){
      $this->fs->mkdir($tmpdir);
    }

    $datestamp = date('c');
    $tmp_output_file = tempnam($tmpdir, 'task.' . $datestamp . '.output.');
    $tmp_error_file = tempnam($tmpdir, 'task.' . $datestamp . '.error.');

    $effective_wd = $dir? $dir:
      $this->type == 'server'? $this->getProperty('server_config_path'):
      $this->getProperty('root')
    ;

    if ($this->getProvision()->getOutput()->isVerbose()) {
      $this->getProvision()->io()->commandBlock($command, $effective_wd);
    }

    // Output and Errors to files.
    $command .= "> $tmp_output_file 2> $tmp_error_file";

    chdir($effective_wd);
    exec($command, $output, $exit);
    chdir($cwd);

    $stderr = file_get_contents($tmp_error_file);
    $stdout = file_get_contents($tmp_output_file);

    if (!empty($stdout)){
      if ($this->getProvision()->getOutput()->isVerbose()) {
        $this->getProvision()->io()->outputBlock($stdout);
      }
    }

    if ($exit != ResultData::EXITCODE_OK) {
      throw new \Exception($stderr);
    }

    return ${$return};
  }

  /**
   * @param $command
   * @param null $dir
   * @param string $return
   */
  public function process_exec($command, $dir = NULL) {

    $process = new Process($command);
    $env = getenv();

    $env['PROVISION_CONTEXT'] = $this->name;
    $env['PROVISION_CONTEXT_CONFIG_FILE'] = $this->config_path;

    foreach ($this->services as $service_type => $service) {
      $env['PROVISION_CONTEXT_SERVER_' . strtoupper($service_type)] = $service->provider->name;
      $env['PROVISION_CONTEXT_SERVER_' . strtoupper($service_type) . '_CONFIG_PATH'] = $service->provider->server_config_path;
    }

    $process->setEnv($env);
    if ($dir){
      $process->setWorkingDirectory($dir);
    }
    $io = $this->getProvision()->io();
    $process->run(function ($type, $buffer) use ($io) {
        $io->outputBlock(trim($buffer));
    });
    return $process->getExitCode();
  }

    /**
     * return list of command classes.
     */
    function getServiceCommandClasses() {
        $classes = [];
        foreach ($this->servicesInvoke('getCommandClasses') as $class) {
            $classes += $class;
        }
        return $classes;
    }
}

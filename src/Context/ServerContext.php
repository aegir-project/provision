<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Context;
use Consolidation\AnnotatedCommand\CommandFileDiscovery;
use Drupal\Console\Core\Style\DrupalStyle;
use Symfony\Component\Config\Definition\Builder\TreeBuilder;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class ServerContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_server
 */
class ServerContext extends Context implements ConfigurationInterface
{
    /**
     * @var string
     * 'server', 'platform', or 'site'.
     */
    public $type = 'server';

    /**
     * @var array
     * A list of services needed for this context.
     */
    protected $services = [];

    /**
     * ServerContext constructor.
     *
     * @param $name
     * @param $console_config
     * @param array $options
     */
    function __construct($name, $console_config, array $options = [])
    {
        parent::__construct($name, $console_config, $options);
        if (isset($this->config['services'])) {
            $this->prepareServices();
        }
        else {
          $this->services = [];
        }
    }
  
    /**
     * Load Service classes from config into Context..
     */
    protected function prepareServices() {
        foreach ($this->config['services'] as $service_name => $service) {
            $service_name = ucfirst($service_name);
            $service_type = ucfirst($service['type']);
            $service_class = "\\Aegir\\Provision\\Service\\{$service_name}\\{$service_name}{$service_type}Service";
            $this->services[strtolower($service_name)] = new $service_class($service, $this);
        }
    }

    static function option_documentation()
    {
        $options = [
          'remote_host' => 'server: host name; default localhost',
          'script_user' => 'server: OS user name; default current user',
          'aegir_root' => 'server: Aegir root; default '.getenv('HOME'),
          'master_url' => 'server: Hostmaster URL',
        ];

        return $options;
    }

    /**
    * Loads all available \Aegir\Provision\Service classes
    *
    * @return array
    */
    public function getAvailableServices($service = NULL) {

        // Load all service classes
        $classes = [];
        $discovery = new CommandFileDiscovery();
        $discovery->setSearchPattern('*Service.php');
        $servicesFiles = $discovery->discover(__DIR__ .'/../Service', '\Aegir\Provision\Service');

        foreach ($servicesFiles as $serviceClass) {
          $classes[$serviceClass::SERVICE] = $serviceClass;
        }

        if ($service && isset($classes[$service])) {
          return $classes[$service];
        }
        elseif ($service && !isset($classes[$service])) {
          throw new \Exception("No service with name $service was found.");
        }
        else {
          return $classes;
        }
    }

    /**
    * Lists all available services as a simple service => name array.
    * @return array
    */
    public function getServiceOptions() {
        $options = [];
        $services = $this->getAvailableServices();
        foreach ($services as $service => $class) {
            $options[$service] = $class::SERVICE_NAME;
        }
        return $options;
    }

    /**
    * @return array
    */
    protected function getAvailableServiceTypes($service, $service_type = NULL) {

        // Load all service classes
        $classes = [];
        $discovery = new CommandFileDiscovery();
        $discovery->setSearchPattern(ucfirst($service) . '*Service.php');
        $serviceTypesFiles = $discovery->discover(__DIR__ .'/../Service/' . ucfirst($service), '\Aegir\Provision\Service\\' . ucfirst($service));
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
    public function getServiceTypeOptions($service) {
        $options = [];
        $service_types = $this->getAvailableServiceTypes($service);
        foreach ($service_types as $service_type => $class) {
            $options[$service_type] = $class::SERVICE_TYPE_NAME;
        }
        return $options;
    }


  /**
     * Return all services for this context.
     *
     * @return array
     */
    public function getServices() {
        return $this->services;
    }

    /**
     * Pass server specific config to Context configTreeBuilder.
     */
    public function configTreeBuilder(&$root_node)
    {
        $root_node
            ->children()
                ->arrayNode('services')
                    ->prototype('array')
                    ->children()
                        ->scalarNode('type')
                        ->isRequired(true)
                    ->end()
                    ->append($this->addServiceProperties())
                ->end()
            ->end();
    }

    /**
     * Append Service class options_documentation to config tree.
     */
    public function addServiceProperties()
    {
        $builder = new TreeBuilder();
        $node = $builder->root('properties');

        // Load config tree from Service type classes
        if (!empty($this->getProperty('services')) && !empty($this->getProperty('services'))) {
            foreach ($this->getProperty('services') as $service => $info) {
                $service = ucfirst($service);
                $service_type = ucfirst($info['type']);
                $class = "\Aegir\Provision\Service\\{$service}\\{$service}{$service_type}Service";
                foreach ($class::option_documentation() as $name => $description) {
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

    public function verify() {

        // Run verify method on all services.
        foreach ($this->getServices() as $service) {
            $service->verify();
        }

        return "Server Context Verified: " . $this->name;
    }

    /**
     * Output a list of all services for this service.
     */
    public function showServices(DrupalStyle $io) {
        if (!empty($this->getServices())) {
            $rows = [];
            foreach ($this->getServices() as $name => $service) {
                $rows[] = [$name, $service->type];

                // Show all properties.
                if (!empty($service->properties )) {
                    foreach ($service->properties as $name => $value) {
                        $rows[] = ['  ' . $name, $value];
                    }
                }
            }
            $io->table(['Services'], $rows);
        }
    }
}

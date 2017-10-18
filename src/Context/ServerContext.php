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
            $this->services = $this->config['services'];
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
    protected function getAvailableServices($service = NULL) {

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
     * {@inheritdoc}
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
            ->end()
        ->end();
    }

    public function verify() {

//        parent::verify();
        return "Server Context Verified: " . $this->name;
    }

    /**
     * Output a list of all services for this service.
     */
    public function showServices(DrupalStyle $io) {
        if (!empty($this->getServices())) {
            $rows = [];
            foreach ($this->getServices() as $name => $service) {
                $rows[] = [$name, $service['type']];
            }
            $io->table(['Services'], $rows);
        }
    }
}

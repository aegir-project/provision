<?php
/**
 * @file
 * Provides the Aegir\Provision\ServiceProvider class.
 *

 *
 */

namespace Aegir\Provision;

/**
 * Class ServiceProvider
 *
 * Context class for a provider of services. Typically ServerContext
 * 
 * @package Aegir\Provision
 */
class ServiceProvider extends Context
{
    const ROLE = 'provider';

    /**
     * Load Service classes from config into Context.
     */
    protected function prepareServices() {
        foreach ($this->config['services'] as $service_name => $service) {
            $service_class = Service::getClassName($service_name, $service['type']);
            $this->services[$service_name] = new $service_class($service, $this);
        }
    }

    /**
     * Loads service properties into the config tree.
     * @param $root_node
     */
    protected function servicesConfigTree(&$root_node) {
        $root_node
            ->attribute('context', $this)
            ->children()
            ->arrayNode('services')
            ->prototype('array')
            ->children()
            ->scalarNode('type')
            ->isRequired(true)
            ->end()
            ->append($this->addServiceProperties('services'))
            ->end()
            ->end();
    }
}

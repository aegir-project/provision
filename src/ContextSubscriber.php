<?php
/**
 * @file
 * Provides the Aegir\Provision\ContextSubscriber class.
 *

 *
 */

namespace Aegir\Provision;

/**
 * Class ContextSubscriber
 *
 * Context class for those consuming services, typically sites & platforms.
 *
 * @package Aegir\Provision
 */
class ContextSubscriber extends Context
{
    const ROLE = 'subscriber';
    
    /**
     * @var array
     * A list of services this context subscribes to.
     */
    protected $serviceSubscriptions = [];
    
    /**
     * Load ServiceSubscription classes from config into Context..
     */
    protected function prepareServices() {
        if (!isset($this->properties['service_subscriptions']) || count($this->properties['service_subscriptions']) == 0) {
            return;
        }

        foreach ($this->properties['service_subscriptions'] as $service_name => $service) {

            // Load into serviceSubscriptions property.
            $this->serviceSubscriptions[$service_name] = new ServiceSubscription($this, $service['server'], $service_name);

            // Also load into services property for easy access.
            $this->services[$service_name] = $this->serviceSubscriptions[$service_name]->service;
        }
    }
    
    /**
     * Return all subscription for this context.
     *
     * @return array
     */
    public function getSubscriptions() {
        return $this->serviceSubscriptions;
    }
    
    /**
     * Return a specific service subscription from this context.
     *
     * @param $type
     *
     * @return \Aegir\Provision\ServiceSubscription
     * @throws \Exception
     */
    public function getSubscription($type) {
        if (isset($this->serviceSubscriptions[$type])) {
            return $this->serviceSubscriptions[$type];
        }
        else {
            throw new \Exception("Service subscription '$type' does not exist in the context '{$this->name}'.");
        }
    }
    
    /**
     * Return an array of Service classes.
     * @return array
     */
    public function getServices() {
        $services = [];
        foreach ($this->getSubscriptions() as $service => $subscription) {
            $services[$service] = $subscription->service;
        }
        return $services;
    }
    
    protected function servicesConfigTree(&$root_node) {
        $root_node
            ->attribute('context', $this)
            ->children()
                ->arrayNode('service_subscriptions')
                ->prototype('array')
                    ->children()
                    ->setNodeClass('context', 'Aegir\Provision\ConfigDefinition\ContextNodeDefinition')
                    ->node('server', 'context')
                        ->isRequired()
                        ->attribute('context_type', 'server')
                    ->end()
                    ->append($this->addServiceProperties('service_subscriptions'))
                ->end()
            ->end();
    }
}

<?php

namespace Aegir\Provision\Console;

use Aegir\Provision\Common\ProvisionAwareTrait;
use Symfony\Component\Config\Definition\Builder\ScalarNodeDefinition;
use Symfony\Component\Config\Definition\Exception\InvalidConfigurationException;

/**
 * Class ConfigContextNodeDefinition
 *
 * Provides another config "node type" called "context" that validates
 * as an available context.
 *
 * Usage:
 *
 *   $root_node
 *      ->children()
 *          ->setNodeClass('context', 'Aegir\Provision\ConfigDefinition\ConfigContextNodeDefinition')

 *          ->node('web_server', 'context')
 *      ->end()
 *  ->end();
 *
 * Many thanks to @andytruong for the guidance on this class: https://stackoverflow.com/a/25518962
 *
 * @package Aegir\Provision\ConfigDefinition
 */
class ConfigContextNodeDefinition extends ScalarNodeDefinition
{
    use ProvisionAwareTrait;

    protected function createNode()
    {
        /**
         * Override parent::createNode() to add our validator.
         */
        $node = parent::createNode();
        $node->setFinalValidationClosures([
            [$this, 'validateContext']
        ]);
        return $node;
    }

    /**
     * Check if:
     *   - The context exists for this name.
     *   - The type of context is correct.
     *   - The required service exists in the context.
     *
     * @param $value
     */
    public function validateContext($value)
    {
        $this->setProvision($this->getNode()->getAttribute('provision'));
        
        // No need to do anything else.
        // If there is no context named $value, getContext() throws an exception for us.

        // If context_type is specified, Validate that the desired context is the right type.
        if ($this->getNode()->getAttribute('context_type') && $this->getProvision()->getContext($value)->type != $this->getNode()->getAttribute('context_type')) {
            throw new InvalidConfigurationException(strtr('The context specified for !name must be type !type.', [
                '!name' => $this->name,
                '!type' => $this->getNode()->getAttribute('context_type'),
            ]));
        }

        // If service_requirement is specified, or item is in service_subscription, validate that the context has the service available.
        $path = explode('.', $this->getNode()->getPath());
        if ($this->getNode()->getAttribute('service_requirement') || $path[1] == 'service_subscription') {
            $service = $this->getNode()->getAttribute('service_requirement')?
                $this->getNode()->getAttribute('service_requirement'):
                $path[2]
            ;
    
            $this->getProvision()->getContext($value)->getService($service);
        }
        return $value;
    }
}
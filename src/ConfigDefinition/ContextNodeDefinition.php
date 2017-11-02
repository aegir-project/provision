<?php

namespace Aegir\Provision\ConfigDefinition;

use Symfony\Component\Config\Definition\Builder\ScalarNodeDefinition;
use Symfony\Component\Config\Definition\Exception\InvalidConfigurationException;

/**
 * Class ContextNodeDefinition
 *
 * Provides another config "node type" called "context" that validates
 * as an available context.
 *
 * Usage:
 *
 *   $root_node
 *      ->children()
 *          ->setNodeClass('context', 'Aegir\Provision\ConfigDefinition\ContextNodeDefinition')

 *          ->node('web_server', 'context')
 *      ->end()
 *  ->end();
 *
 * @package Aegir\Provision\ConfigDefinition
 */
class ContextNodeDefinition extends ScalarNodeDefinition
{
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
        // No need to do anything else.
        // If there is no context named $value, getContext() throws an exception for us.
        $this->parent->getAttribute('application')->getContext($value);

        // If context_type is specified, Validate that the desired context is the right type.
        if ($this->getNode()->getAttribute('context_type') && $this->parent->getAttribute('application')->getContext($value)->type != $this->getNode()->getAttribute('context_type')) {
            throw new InvalidConfigurationException(strtr('The context specified for !name must be type !type.', [
                '!name' => $this->name,
                '!type' => $this->getNode()->getAttribute('context_type'),
            ]));
        }

        // If service_requirement is specified, validate that the context has the service.
        if ($this->getNode()->getAttribute('service_requirement')) {
            $service = $this->getNode()->getAttribute('service_requirement');

            try {
                $this->parent->getAttribute('application')->getContext($value)->getService($service);
            }
            catch (\Exception $e) {
                throw new \Exception("Service '$service' does not exist in the specified context '$value'.");
            }
        }
    }
}
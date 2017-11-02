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
     * Check if the context exists for this name.
     *
     * @param $value
     */
    public function validateContext($value)
    {
        // No need to do anything else.
        // If there is no context named $value, getContext() throws an exception for us.
        $this->parent->getAttribute('application')->getContext($value);
    }
}
<?php

namespace Aegir\Provision;

use Consolidation\Config\ConfigInterface;
use League\Container\ContainerAwareInterface;
use League\Container\ContainerAwareTrait;
use Psr\Log\LoggerAwareInterface;
use Psr\Log\LoggerAwareTrait;
use Robo\Collection\CollectionBuilder;
use Robo\Common\ConfigAwareTrait;
use Robo\Common\IO;
use Robo\Contract\BuilderAwareInterface;
use Robo\Contract\ConfigAwareInterface;
use Robo\Contract\IOAwareInterface;
use Robo\Task\Base\loadTasks;

/**
 * Base class for BLT Robo commands.
 */
class ProvisionTasks implements ConfigAwareInterface, LoggerAwareInterface, BuilderAwareInterface, IOAwareInterface, ContainerAwareInterface
{
    
    use ContainerAwareTrait;
    use loadTasks;
    
    use ConfigAwareTrait;
//    use InspectorAwareTrait;
    use IO;
    use LoggerAwareTrait;
    
    /**
     * Set the config reference
     *
     * @param ConfigInterface $config
     *
     * @return $this
     */
    public function setConfig(ConfigInterface $config) {
        $this->config = $config;
    }
    
    /**
     * Get the config reference
     *
     * @return ConfigInterface
     */
    public function getConfig() {
        return $this->config;
    }
    
    public function getBuilder()
    {
        return $this->builder;
    }

    public function setBuilder(CollectionBuilder $builder)
    {
        $this->builder = $builder;
    }
}

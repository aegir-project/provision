<?php

namespace Aegir\Provision\Robo;

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
use Robo\LoadAllTasks;

/**
 * Base class for BLT Robo commands.
 */
class ProvisionTasks implements ConfigAwareInterface, LoggerAwareInterface, BuilderAwareInterface, IOAwareInterface, ContainerAwareInterface {
    
    use ContainerAwareTrait;
    use LoadAllTasks;
    use ConfigAwareTrait;
//    use InspectorAwareTrait;
    use IO;
    use LoggerAwareTrait;
}

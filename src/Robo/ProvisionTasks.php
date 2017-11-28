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

use Robo\Task\Docker\Build;
use Robo\Task\Docker\Commit;
use Robo\Task\Docker\Exec;
use Robo\Task\Docker\Pull;
use Robo\Task\Docker\Remove;
use Robo\Task\Docker\Run;
use Robo\Task\Docker\Start;
use Robo\Task\Docker\Stop;

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
    
    /**
     * Public Overrides
     */
    
    /**
     * @param string $image
     *
     * @return \Robo\Task\Docker\Run
     */
    public function taskDockerRun($image)
    {
        return $this->task(Run::class, $image);
    }
    
    /**
     * @param string $image
     *
     * @return \Robo\Task\Docker\Pull
     */
    public function taskDockerPull($image)
    {
        return $this->task(Pull::class, $image);
    }
    
    /**
     * @param string $path
     *
     * @return \Robo\Task\Docker\Build
     */
    public function taskDockerBuild($path = '.')
    {
        return $this->task(Build::class, $path);
    }
    
    /**
     * @param string|\Robo\Task\Docker\Result $cidOrResult
     *
     * @return \Robo\Task\Docker\Stop
     */
    public function taskDockerStop($cidOrResult)
    {
        return $this->task(Stop::class, $cidOrResult);
    }
    
    /**
     * @param string|\Robo\Task\Docker\Result $cidOrResult
     *
     * @return \Robo\Task\Docker\Commit
     */
    public function taskDockerCommit($cidOrResult)
    {
        return $this->task(Commit::class, $cidOrResult);
    }
    
    /**
     * @param string|\Robo\Task\Docker\Result $cidOrResult
     *
     * @return \Robo\Task\Docker\Start
     */
    public function taskDockerStart($cidOrResult)
    {
        return $this->task(Start::class, $cidOrResult);
    }
    
    /**
     * @param string|\Robo\Task\Docker\Result $cidOrResult
     *
     * @return \Robo\Task\Docker\Remove
     */
    public function taskDockerRemove($cidOrResult)
    {
        return $this->task(Remove::class, $cidOrResult);
    }
    
    /**
     * @param string|\Robo\Task\Docker\Result $cidOrResult
     *
     * @return \Robo\Task\Docker\Exec
     */
    public function taskDockerExec($cidOrResult)
    {
        return $this->task(Exec::class, $cidOrResult);
    }
}

<?php

namespace Aegir\Provision\Robo;

use Aegir\Provision\Robo\Task\Log;
use Consolidation\Config\ConfigInterface;
use League\Container\ContainerAwareInterface;
use League\Container\ContainerAwareTrait;
use Psr\Log\LoggerAwareInterface;
use Psr\Log\LoggerAwareTrait;
use Psr\Log\LoggerInterface;
use Psr\Log\LoggerTrait;
use Psr\Log\LogLevel;
use Robo\Collection\CollectionBuilder;
use Robo\Common\ConfigAwareTrait;
use Robo\Common\IO;
use Robo\Contract\BuilderAwareInterface;
use Robo\Contract\ConfigAwareInterface;
use Robo\Contract\IOAwareInterface;
use Robo\LoadAllTasks;

use Robo\Task\Base\Exec;
use Robo\Task\Base\ExecStack;
use Robo\Task\Base\ParallelExec;
use Robo\Task\Base\SymfonyCommand;
use Robo\Task\Base\Watch;
use Robo\Task\Docker\Build;
use Robo\Task\Docker\Commit;
use Robo\Task\Docker\Exec as DockerExec;
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
     * @param string|\Robo\Contract\CommandInterface $command
     *
     * @return Log
     */
    public function taskLog($message, $level = LogLevel::INFO) {
        
        return $this->task(Log::class, $this->logger, $message, $level);
    }
    
    /**
     * Public Overrides
     */
    
    /**
     * @param string|\Robo\Contract\CommandInterface $command
     *
     * @return Exec
     */
    public function taskExec($command)
    {
        return $this->task(\Robo\Task\Base\Exec::class, $command);
    }
    
    /**
     * @return ExecStack
     */
    public function taskExecStack()
    {
        return $this->task(ExecStack::class);
    }
    
    /**
     * @return ParallelExec
     */
    public function taskParallelExec()
    {
        return $this->task(ParallelExec::class);
    }
    
    /**
     * @param $command
     * @return SymfonyCommand
     */
    public function taskSymfonyCommand($command)
    {
        return $this->task(SymfonyCommand::class, $command);
    }
    
    /**
     * @return Watch
     */
    public function taskWatch()
    {
        return $this->task(Watch::class, $this);
    }
    
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
        return $this->task(DockerExec::class, $cidOrResult);
    }
}

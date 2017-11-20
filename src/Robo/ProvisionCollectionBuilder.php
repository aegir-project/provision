<?php

namespace Aegir\Provision\Robo;

use Psr\Log\LoggerAwareInterface;
use Psr\Log\LoggerAwareTrait;
use Psr\Log\LoggerInterface;
use Psr\Log\LoggerTrait;
use Robo\Collection\CollectionBuilder;
use Robo\Collection\NestedCollectionInterface;
use Robo\Common\CommandArguments;
use Robo\Common\ExecCommand;
use Robo\Common\ExecTrait;
use Robo\Common\OutputAdapter;
use Robo\Common\TaskIO;
use Robo\Result;
use Robo\ResultData;
use Symfony\Component\Process\Process;

class ProvisionCollectionBuilder extends CollectionBuilder {
    
    /**
     * @var string|\Robo\Contract\CommandInterface
     */
    protected $command;
    
    /**
     * @var \Robo\Contract\TaskInterface
     */
    protected $currentTask;
    
}
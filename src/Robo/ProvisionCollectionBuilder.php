<?php

namespace Aegir\Provision\Robo;

use Aegir\Provision\Common\ProvisionAwareTrait;
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
use Robo\Config\Config;
use Robo\Result;
use Robo\ResultData;
use Symfony\Component\Process\Process;

class ProvisionCollectionBuilder extends CollectionBuilder {
    
    use ProvisionAwareTrait;
    
    /**
     * Return the collection of tasks associated with this builder.
     *
     * @return \Robo\Collection\CollectionInterface
     */
    public function getCollection()
    {
        if (!isset($this->collection)) {
            $this->collection = new ProvisionCollection();
            $this->collection->setProvision($this->getProvision());
            $this->collection->inflect($this);
            $this->collection->setState($this->getState());
            $this->collection->setProgressBarAutoDisplayInterval($this->getConfig()->get(Config::PROGRESS_BAR_AUTO_DISPLAY_INTERVAL));
            
            if (isset($this->currentTask)) {
                $this->collection->add($this->currentTask);
            }
        }
        return $this->collection;
    }
}
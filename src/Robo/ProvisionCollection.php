<?php

namespace Aegir\Provision\Robo;

use Aegir\Provision\Common\ProvisionAwareTrait;
use Robo\Collection\Collection;
use Robo\Exception\TaskExitException;
use Robo\Result;

class ProvisionCollection extends Collection {
    
    use ProvisionAwareTrait;
    
    /**
     * Run our tasks, and roll back if necessary.
     *
     * @return \Robo\Result
     */
    public function run()
    {
        $this->disableProgressIndicator();
        $result = $this->runWithoutCompletion();
        $this->complete();
        return $result;
    }
    
    /**
     * @return \Robo\Result
     */
    private function runWithoutCompletion()
    {
        $result = Result::success($this);
        
        if (empty($this->taskList)) {
            return $result;
        }
        
        if ($result->wasSuccessful()) {
            foreach ($this->taskList as $name => $taskGroup) {
    
                if ($this->getProvision()->getOutput()->isVerbose()) {
                    $this->getProvision()->io()->customLite('STARTED ' . $name, '○');
                }
                
                // ROBO
                $taskList = $taskGroup->getTaskList();
                $result = $this->runTaskList($name, $taskList, $result);
                // END ROBO
    
                if (!$result->wasSuccessful()) {
                    if (!empty($this->getConfig()->get($name . '.failure'))) {
                        $name = $this->getConfig()->get($name . '.failure');
                    }
    
                    if ($this->getProvision()->getOutput()->isVerbose()) {
                        $this->getProvision()->io()->errorLite('<options=bold>FAILED </> ' . $name);
                    }
                    else {
                        $this->getProvision()->io()->errorLite($name);
                    }
                    $this->fail();
                    return $result;
                }
                else {
                    
                    // Skip the logging tasks.
                    if (strpos($name, 'logging.') === 0) {
                        continue;
                    }
    
                    if (!empty($this->getConfig()->get($name . '.success'))) {
                        $name = $this->getConfig()->get($name . '.success');
                    }
                    if ($this->getProvision()->getOutput()->isVerbose()) {
                        $this->getProvision()->io()->successLite('<fg=green>SUCCESS</> '.$name);
                    }
                    else {
                        $this->getProvision()->io()->successLite($name);
                    }
                }
            }
            $this->taskList = [];
        }
        $result['time'] = $this->getExecutionTime();
        
        return $result;
    }
    
    /**
     * {@inheritdoc}
     *
     * An exact copy of Collection::runTaskList(), because it is private and we need access.
     */
    private function runTaskList($name, array $taskList, Result $result)
    {
        try {
            foreach ($taskList as $taskName => $task) {
                $taskResult = $this->runSubtask($task);
                // If the current task returns an error code, then stop
                // execution and signal a rollback.
                if (!$taskResult->wasSuccessful()) {
                    return $taskResult;
                }
                // We accumulate our results into a field so that tasks that
                // have a reference to the collection may examine and modify
                // the incremental results, if they wish.
                $key = Result::isUnnamed($taskName) ? $name : $taskName;
                $result->accumulate($key, $taskResult);
                // The result message will be the message of the last task executed.
                $result->setMessage($taskResult->getMessage());
            }
        } catch (TaskExitException $exitException) {
            $this->fail();
            throw $exitException;
        } catch (\Exception $e) {
            // Tasks typically should not throw, but if one does, we will
            // convert it into an error and roll back.
            return Result::fromException($task, $e, $result->getData());
        }
        return $result;
    }
}
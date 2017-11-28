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
        
        $this->startProgressIndicator();
        if ($result->wasSuccessful()) {
            foreach ($this->taskList as $name => $taskGroup) {
                $taskList = $taskGroup->getTaskList();
                $result = $this->runTaskList($name, $taskList, $result);
                if (!$result->wasSuccessful()) {
                    $this->fail();
                    return $result;
                }
            }
            $this->taskList = [];
        }
        $this->stopProgressIndicator();
        $result['time'] = $this->getExecutionTime();
        
        return $result;
    }
    
    /**
     * Run every task in a list, but only up to the first failure.
     * Return the failing result, or success if all tasks run.
     *
     * @param string $name
     * @param TaskInterface[] $taskList
     * @param \Robo\Result $result
     *
     * @return \Robo\Result
     *
     * @throws \Robo\Exception\TaskExitException
     */
    private function runTaskList($name, array $taskList, Result $result)
    {
        try {
            foreach ($taskList as $taskName => $task) {
                $this->getProvision()->io()->customLite($name, '☐');
                $taskResult = $this->runSubtask($task);
                $this->advanceProgressIndicator();
                // If the current task returns an error code, then stop
                // execution and signal a rollback.
                if (!$taskResult->wasSuccessful()) {
                    $this->getProvision()->io()->errorLite($name);
                    return $taskResult;
                }
                // We accumulate our results into a field so that tasks that
                // have a reference to the collection may examine and modify
                // the incremental results, if they wish.
                $key = Result::isUnnamed($taskName) ? $name : $taskName;
                $result->accumulate($key, $taskResult);
                // The result message will be the message of the last task executed.
                $result->setMessage($taskResult->getMessage());
                
                $this->getProvision()->io()->successLite($name);
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
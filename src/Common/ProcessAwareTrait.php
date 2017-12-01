<?php

namespace Aegir\Provision\Common;

use Aegir\Provision\Context;
use Symfony\Component\Process\Process;
use Twig\Node\Expression\Unary\NegUnary;

trait ProcessAwareTrait
{
    /**
     * @var Process
     */
    protected $process = NULL;

//    /**
//     * @var string
//     */
//    protected $command;

    /**
     * @param Process $process
     *
     * @return $this
     */
    protected function setProcess(Process $process = NULL)
    {
        $this->process = $process;
        return $this;
    }

    /**
     * @return Process
     */
    public function getProcess()
    {
        if (is_null($this->process)) {
            $this->process = new Process($this->command);
        }
        return $this->process;

    }

    /**
     * @param $command
     *
     * @return $this
     */
    public function setCommand($command) {

        $this->command = $command;

        return $this;
    }

    /**
     * @return string
     */
    public function getCommand() {

        return $this->command;
    }

    public function execute() {
        $this->process->run();
    }

}

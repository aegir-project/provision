<?php
namespace Aegir\Provision\Robo\Task;

use Psr\Log\LoggerAwareTrait;
use Psr\Log\LoggerInterface;
use Psr\Log\LogLevel;
use Robo\Common\ExecTrait;
use Robo\Contract\CommandInterface;
use Robo\Contract\PrintedInterface;
use Robo\Contract\SimulatedInterface;
use Robo\Task\BaseTask;
use Symfony\Component\Process\Process;
use Robo\Result;

/**
 * Logs output.
 *
 * ``` php
 * <?php
 * $this->taskLog('Hello, World');
 * ?>
 * ```
 */
class Log extends BaseTask
{
    use LoggerAwareTrait;
    
    /**
     * Log constructor.
     *
     * @param \Psr\Log\LoggerInterface $logger
     * @param string                   $message
     */
    public function __construct(LoggerInterface $logger, $message, $level = LogLevel::INFO)
    {
        $this->setLogger($logger);
        $this->message = $message;
        $this->level = $level;
    }
    
    public function run() {
        $this->logger()->log($this->level, $this->message);
    }

}
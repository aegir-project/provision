<?php

namespace Aegir\Provision\Service\Http\Php;

use Robo\Common\ExecCommand;
use Robo\Common\ExecTrait;

class PhpServer extends \Robo\Task\Development\PhpServer {


    /**
     * @var int
     */
    protected $port;

    /**
     * @var string
     */
    protected $host = '127.0.0.1';

    /**
     * @var string
     */
    protected $command = 'php -S %s:%d ';

    /**
     * @param int $port
     */
    public function __construct($port)
    {
        $this->port = $port;

        if (strtolower(PHP_OS) === 'linux') {
            $this->command = 'exec php -S %s:%d ';
        }
    }

    /**
     * @param string $host
     *
     * @return $this
     */
    public function host($host)
    {
        $this->host = $host;
        return $this;
    }

    /**
     * @param string $path
     *
     * @return $this
     */
    public function dir($path)
    {
        $this->command .= "-t $path";
        return $this;
    }

    /**
     * {@inheritdoc}
     */
    public function getCommand()
    {
        return sprintf($this->command, $this->host, $this->port);
    }

    public function run() {
        $this->executeCommand($this->getCommand());
    }

    public function getProcess() {
        return $this->process;
    }

    public function task() {

    }
}
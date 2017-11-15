<?php

namespace Aegir\Provision\Console;

use BennerInformatics\Spinner\ProcessSpinner;
use Symfony\Component\Config\Definition\Exception\Exception;
use Symfony\Component\Console\Output\ConsoleOutput as BaseConsoleOutput;
use Symfony\Component\Process\Process;

/**
 * Class Config.
 */
class ConsoleOutput extends BaseConsoleOutput
{
    
    private $firstRun = true;
    protected $process;
    protected $spinFrames = ['/', '-', '\\', '|'];
    protected $spinInterval = 85000;
    
    /**
     * Overwrites a previous message to the output.
     *
     * @param string $message The message
     */
    public function erase($lines = 1)
    {
            if (!$this->firstRun) {
                // Move the cursor to the beginning of the line
                $this->write("\x0D");
                
                // Erase the line
                $this->write("\x1B[2K");
                
                // Erase previous lines
                if ($lines > 0) {
                    $this->write(str_repeat("\x1B[1A\x1B[2K", $lines));
                }
        }
        $this->firstRun = false;
    }
    
    /**
     * Inspired by https://github.com/BennerInformatics/php-process-spinner/
     * @param        $cmd
     * @param string $start_message
     * @param string $end_message
     *
     * @return bool
     */
    public function exec($cmd, $start_message = 'Running command...', $end_message = 'Command complete.') {
        $spinPos = 0;
        $this->process = new Process($cmd);
        $this->process->start();
        while ($this->process->isRunning()) {
            $this->write(" <comment>{$this->spinFrames[$spinPos]} </comment>{$start_message}\r");
            $spinPos = ($spinPos + 1) % count($this->spinFrames);
            usleep($this->spinInterval);
        }
        
        if ($this->process->isSuccessful()) {
            $this->writeln($end_message);
            return true;
        }
        else {
            throw new Exception("Running command {$cmd} failed: " . $this->process->getErrorOutput());
        }
    }
}

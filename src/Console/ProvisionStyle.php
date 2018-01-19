<?php

namespace Aegir\Provision\Console;

use Drupal\Console\Core\Style\DrupalStyle;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\BufferedOutput;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Console\Terminal;

class ProvisionStyle extends DrupalStyle {

    /**
     * @var BufferedOutput
     */
    protected $bufferedOutput;
    protected $input;
    protected $lineLength;

    const TERMINAL_COMMAND_INDICATOR = '$';

    public function __construct(InputInterface $input, OutputInterface $output)
    {
        $this->input = $input;
        $this->bufferedOutput = new BufferedOutput($output->getVerbosity(), false, clone $output->getFormatter());
        // Windows cmd wraps lines as soon as the terminal width is reached, whether there are following chars or not.
        $width = (new Terminal())->getWidth() ?: self::MAX_LINE_LENGTH;
        $this->lineLength = min($width - (int) (DIRECTORY_SEPARATOR === '\\'), self::MAX_LINE_LENGTH);

        parent::__construct($input, $output);
    }


    public function commandBlock($message) {
        $this->autoPrependBlock();
        $this->customLite($message, '<fg=yellow>' . self::TERMINAL_COMMAND_INDICATOR . '</>', '');
    }

    public function outputBlock($message) {
        $this->block(
            $message,
            NULL,
            'fg=yellow;bg=black',
            ' ╎ ',
            TRUE
            );
    }

    /**
     * Replacement for parent::autoPrependBlock(), allowing access and setting newLine to 1 - instead of 2 -.
     */
    private function autoPrependBlock()
    {
        $chars = substr(str_replace(PHP_EOL, "\n", $this->bufferedOutput->fetch()), -2);

        if (!isset($chars[0])) {
            return $this->newLine(); //empty history, so we should start with a new line.
        }
        //Prepend new line for each non LF chars (This means no blank line was output before)
        $this->newLine(1 - substr_count($chars, "\n"));
    }
}
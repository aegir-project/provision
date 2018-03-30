<?php

// We use PWD if available because getcwd() resolves symlinks, which
// could take us outside of the Drupal root, making it impossible to find.
$cwd = empty($_SERVER['PWD']) ? getcwd() : $_SERVER['PWD'];

// Set up autoloader
$loader = false;
if (file_exists($autoloadFile = __DIR__ . '/vendor/autoload.php')
    || file_exists($autoloadFile = __DIR__ . '/../vendor/autoload.php')
    || file_exists($autoloadFile = __DIR__ . '/../autoload.php')
    || file_exists($autoloadFile = __DIR__ . '/../../autoload.php')
) {
    $loader = include_once($autoloadFile);
} else {
    throw new \Exception("Could not locate autoload.php. cwd is $cwd; __DIR__ is " . __DIR__);
}

use Aegir\Provision\Console\ConsoleOutput;
use Aegir\Provision\Console\Config;
use Aegir\Provision\Common\NotSetupException;

use Aegir\Provision\Console\ProvisionStyle;
use Robo\Common\TimeKeeper;
use Aegir\Provision\Console\ArgvInput;
use Symfony\Component\Console\Exception\InvalidOptionException;
use Symfony\Component\Console\Exception\CommandNotFoundException;

// Start Timer.
$timer = new TimeKeeper();
$timer->start();

try {
    // Create input output objects.
    $input = new ArgvInput($argv);
    $output = new ConsoleOutput();
    $io = new ProvisionStyle($input, $output);

    // Try to load config. If there are no files detected, Config::__construct()
    // will throw a NotSetupException.
    // When that happens, run the 'setup' command instead of whatever command was run by overriding $argv.
    try {
        // Create a config object.
        $config = new Config($io);
    }
    catch (NotSetupException $e) {

        if ($e->getMessage()) {
            $io->warningLite('There were problems detected with your system: ');

            $io->outputBlock($e->getMessage());

            $io->warningLite(' Running `provision setup` command to fix the problems... ');
        }

        // Replace given command with "setup", unless there is no command, then append to argv.
        $command = $input->getFirstArgument();
        $command_key = array_search($input->getFirstArgument(), $argv);
        if (empty($command_key)) {
            $argv[] = 'setup';
        }
        else {
            $argv[$command_key] = 'setup';
        }
        $input = new ArgvInput($argv);
        $io = new ProvisionStyle($input, $output);
        $config = new Config($io, FALSE);
    }
    
    // Create the app.
    $provision = new \Aegir\Provision\Provision($config, $input, $output);
    
    // Run the app.
    $status_code = $provision->run($input, $output);
    
}
catch (Exception $e) {
    $io->error("Something went wrong with Provision: " . $e->getMessage(), 1);
    $status_code = 1;
}

// Stop timer.
$timer->stop();
if ($output->isVerbose()) {
    $output->writeln("<comment>" . $timer->formatDuration($timer->elapsed()) . "</comment> total time elapsed.");
}

exit($status_code);

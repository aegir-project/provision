<?php

use League\Container\Container;
use Robo\Robo;
use Robo\Common\TimeKeeper;

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

// Start Timer.
$timer = new TimeKeeper();
$timer->start();

$input = new \Symfony\Component\Console\Input\ArgvInput($argv);
$output = new \Aegir\Provision\Console\ConsoleOutput();

$app = new \Aegir\Provision\Provision($input, $output);

$status_code = $app->run($input, $output);

// Stop timer.
$timer->stop();
if ($output->isVerbose()) {
    $output->writeln("<comment>" . $timer->formatDuration($timer->elapsed()) . "</comment> total time elapsed.");
}

exit($status_code);

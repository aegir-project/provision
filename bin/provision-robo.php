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
use Aegir\Provision\Console\DefaultsConfig;
use Aegir\Provision\Console\DotEnvConfig;
use Aegir\Provision\Console\EnvConfig;
use Aegir\Provision\Console\YamlConfig;
use Robo\Common\TimeKeeper;
use Symfony\Component\Console\Input\ArgvInput;

// Start Timer.
$timer = new TimeKeeper();
$timer->start();

// Create input output objects.
$input = new ArgvInput($argv);
$output = new ConsoleOutput();

// Create a config object.

$config = new DefaultsConfig();
$config->extend(new YamlConfig($config->get('user_home') . '/.provision.yml'));
$config->extend(new DotEnvConfig(getcwd()));
$config->extend(new EnvConfig());

// Create the app.
$app = new \Aegir\Provision\Provision($config, $input, $output);

$status_code = $app->run($input, $output);

// Stop timer.
$timer->stop();
if ($output->isVerbose()) {
    $output->writeln("<comment>" . $timer->formatDuration($timer->elapsed()) . "</comment> total time elapsed.");
}

exit($status_code);

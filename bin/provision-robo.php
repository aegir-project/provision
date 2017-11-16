<?php

use League\Container\Container;
use Robo\Robo;
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

$input = new \Symfony\Component\Console\Input\ArgvInput($argv);
$output = new \Symfony\Component\Console\Output\ConsoleOutput();

$app = new \Aegir\Provision\Provision($input, $output);

$status_code = $app->run($input, $output);
exit($status_code);

<?php

namespace Aegir\Provision;

use Aegir\Provision\Command\SaveCommand;
use Aegir\Provision\Command\ShellCommand;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\Console\Application as BaseApplication;

//use Symfony\Component\DependencyInjection\ContainerInterface;
//use Drupal\Console\Annotations\DrupalCommandAnnotationReader;
//use Drupal\Console\Utils\AnnotationValidator;
//use Drupal\Console\Core\Application as BaseApplication;


/**
 * Class Application
 *
 * @package Drupal\Console
 */
class Application extends BaseApplication
{
  /**
   * @var string
   */
  const NAME = 'Aegir Provision';

  /**
   * @var string
   */
  const VERSION = '4.x';

  /**
   * @var string
   */
  const DEFAULT_TIMEZONE = 'America/New_York';

  public function __construct()
  {

    // If no timezone is set, set Default.
    if (empty(ini_get('date.timezone'))) {
      date_default_timezone_set($this::DEFAULT_TIMEZONE);
    }

    parent::__construct($this::NAME, $this::VERSION);
  }

  /**
   * Initializes all the default commands.
   */
  protected function getDefaultCommands() {
    $commands = parent::getDefaultCommands();
    $commands[] = new SaveCommand();
    $commands[] = new ShellCommand();
    return $commands;
  }
}

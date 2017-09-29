<?php

namespace Aegir\Provision;

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

  public function __construct()
  {
    parent::__construct($this::NAME, $this::VERSION);
  }
}

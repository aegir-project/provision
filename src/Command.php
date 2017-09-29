<?php

namespace Aegir\Provision;

use Symfony\Component\Console\Command\Command as BaseCommand;
use Drupal\Console\Core\Command\Shared\CommandTrait;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class Command
 * @package Aegir\Provision\Command
 */
abstract class Command extends BaseCommand
{
  use CommandTrait;


  /**
   * @var \Symfony\Component\Console\Input\InputInterface
   */
  protected $input;

  /**
   * @var \Symfony\Component\Console\Output\OutputInterface
   */
  protected $output;

  /**
   * @param InputInterface  $input  An InputInterface instance
   * @param OutputInterface $output An OutputInterface instance
   */
  protected function initialize(InputInterface $input, OutputInterface $output)
  {
    $this->input = $input;
    $this->output = $output;
  }

  /**
   * Run a process.
   *
   * @param $cmd
   */
  protected function process($cmd) {
    $this->output->writeln(["Running: $cmd"]);
    shell_exec($cmd);
  }
}

<?php

namespace Aegir\Provision\Service\Http\ApacheDocker;

use Symfony\Component\Console\Input\ArgvInput;

/**
 * This is project's console commands configuration for Robo task runner.
 *
 * @see http://robo.li/
 */
class ApacheDockerCommands extends \Robo\Tasks
{

  public function dockerCompose($docker_compose_command = 'Docker compose command to run.') {
      /** @var \Aegir\Provision\Application $application */
      $application = $this->getContainer()->get('application');

      /** @var \Robo\Log\RoboLogger $logger */
      $logger = $this->getContainer()->get('logger');

      /** @var ArgvInput $input */
      $input = $this->getContainer()->get('input');

      $logger->info('Hi! This is a robo command.');

      $process = new \Symfony\Component\Process\Process("docker-compose {$docker_compose_command}");

      $wd = $this->getContainer()->get('application')->getProvision()->activeContext == 'server'?
        $this->getContainer()->get('application')->getProvision()->activeContext->server_config_path:
        $this->getContainer()->get('application')->getProvision()->activeContext->service('http')->provider->server_config_path;

      $process->setWorkingDirectory($wd);
      $process->setTty(TRUE);
      $process->run();
  }


  /**
   * Stream logs from the containers using docker-compose logs -f
   */
  public function dockerLogs() {
    $process = new \Symfony\Component\Process\Process("docker-compose logs -f");

    $wd = $this->getContainer()->get('application')->getProvision()->activeContext == 'server'?
      $this->getContainer()->get('application')->getProvision()->activeContext->server_config_path:
      $this->getContainer()->get('application')->getProvision()->activeContext->service('http')->provider->server_config_path;

    $process->setWorkingDirectory($wd);
    $process->setTty(TRUE);
    $process->run();  }

  /**
   * Enter a bash shell in the web server container.
   */
  public function dockerShell() {
    $process = new \Symfony\Component\Process\Process("docker-compose exec http bash");

    $wd = $this->getContainer()->get('application')->getProvision()->activeContext == 'server'?
      $this->getContainer()->get('application')->getProvision()->activeContext->server_config_path:
      $this->getContainer()->get('application')->getProvision()->activeContext->service('http')->provider->server_config_path;

    $process->setWorkingDirectory($wd);
    $process->setTty(TRUE);
    $process->run();
  }
}
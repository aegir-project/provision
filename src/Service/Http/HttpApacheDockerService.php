<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Robo\ProvisionExecutor;
use Aegir\Provision\Robo\ProvisionTasks;
use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfiguration;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfiguration;
use Aegir\Provision\Service\Http\ApacheDocker\Configuration\ServerConfiguration;
use Behat\Mink\Exception\Exception;
use Psr\Log\LogLevel;
use Robo\Task\Base\Exec;
use Robo\Task\Docker\Run;
use Robo\Tasks;

/**
 * Class HttpApacheDockerService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpApacheDockerService extends HttpApacheService
{
  const SERVICE_TYPE = 'apacheDocker';
  const SERVICE_TYPE_NAME = 'Apache on Docker';
  
  /**
   * Returns array of Configuration classes for this service.
   *
   * @see Provision_Service_http_apache::init_server();
   *
   * @return array
   */
  public function getConfigurations()
  {
    $configs['server'][] = ServerConfiguration::class;
    $configs['platform'][] = PlatformConfiguration::class;
    $configs['site'][] = SiteConfiguration::class;
    return $configs;
  }
  
  public function verify()
  {
      $provision = $this->getProvision();
      $provider = $this->provider;
      $logger = $this->getProvision()->getLogger();
      $collection = $this->getProvision()->getBuilder();
      $collection->addCode(
          function () use ($logger) {
              $this->writeConfigurations();
          }
      );
      
      $tag = "provision/http:{$this->provider->name}";
      $name = "provision_http_{$this->provider->name}";
      $build_dir = __DIR__ . DIRECTORY_SEPARATOR . '/ApacheDocker/';
      
      # Build Docker Image
      $collection->getCollection()->addCode(function () use ($provision, $tag, $name, $provider, $build_dir) {
          $buildResult = $this->getProvision()->getTasks()->taskDockerBuild($build_dir)
              ->tag($tag)
              ->option(
                  '-f',
                  __DIR__.DIRECTORY_SEPARATOR.'/ApacheDocker/http.Dockerfile'
              )
              ->option(
                  '--build-arg',
                  "AEGIR_SERVER_NAME={$this->provider->name}"
              )
              ->option('--build-arg', "AEGIR_UID=".posix_getuid())
              ->silent(!$this->getProvision()->getOutput()->isVerbose())
              ->run()
          ;
    
          if ($buildResult->wasSuccessful()) {
              $provision->io()->successLite('Built new Docker image for Apache: ' . $tag);
          }
          else {
              $provision->io()->errorLite('Unable to build docker container with tag: ' . $tag);
              throw new \Exception('Unable to build docker container with tag: ' . $tag);
          }
      });
      
      # Run Docker Image
      $collection->getCollection()->addCode(function () use ($provision, $tag, $name, $provider) {
    
          // Check for existing container.
          $containerExists = $provision->getTasks()
              ->taskExec("docker ps -a -q -f name={$name}")
                    ->silent(!$this->getProvision()->getOutput()->isVerbose())
                  ->printOutput(false)
                  ->storeState('container_id')
              ->taskFileSystemStack()
                  ->defer(
                      function ($task, $state) use ($provision, $tag, $name, $provider) {
                          $container = $state['container_id'];
                          if ($container) {
                              
                              //Check that it is running
                              $provision->getTasks()
                                  ->taskExec("docker inspect -f '{{.State.Running}}' {$name}")
                                      ->silent(!$provision->getOutput()->isVerbose())
                                      ->printOutput(false)
                                      ->storeState('running')
                                  ->taskFileSystemStack()
                                      ->defer(
                                          function ($task, $state) use ($provision, $tag, $name, $provider) {
    
                                              if ($state['running'] == 'true') {
                                                  $provision->io()->successLite('Container is already running: ' . $name);
                                              }
                                              // If not running, try to start it.
                                              else {
                                                  $startResult = $provision->getTasks()
                                                      ->taskExec("docker start {$name}")
                                                      ->silent(!$provision->getOutput()->isVerbose())
                                                      ->run()
                                                  ;
        
                                                  if ($startResult->wasSuccessful()) {
                                                      $provision->io()->successLite('Existing container found. Restarted container ' . $name);
                                                  }
                                                  else {
                                                      $provision->io()->errorLite('Unable to restart docker container: ' . $name);
                                                      throw new \Exception('Unable to restart docker container: ' . $name);
                                                  }
                                              }
                                          })
                                  ->run();
                              
                          }
                          
                          # Docker container not found. Start it.
                          else {
                              $configVolumeHost = $provision->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $this->provider->name . DIRECTORY_SEPARATOR . '/apache';
                              $configVolumeGuest = $this->provider->getProperty('aegir_root') . '/config/' . $this->provider->name . DIRECTORY_SEPARATOR . '/apache';
    
                              $result = $provision->getTasks()->taskDockerRun($tag)
                                  ->detached()
                                  ->publish(80)
                                  ->name($name)
                                  ->volume($configVolumeHost, $configVolumeGuest)
                                  ->silent(!$provision->getOutput()->isVerbose())
                                  ->interactive()
                                  ->run();
    
                              if ($result->wasSuccessful()) {
                                  $provision->io()->successLite('Running Docker image ' . $tag);
                              }
                              else {
                                  $provision->io()->errorLite('Unable to run docker container: ' . $name);
                                  throw new \Exception('Unable to run docker container: ' . $name);
                              }
                          }
                      }
                  )
              ->run();
          if ($containerExists->wasSuccessful()) {
              $this->getProvision()->getLogger()->info('All processes completed successfully.');
          }
          else {
              throw new \Exception('Something went wrong.');
          }
      });
      return $collection;
  }
}

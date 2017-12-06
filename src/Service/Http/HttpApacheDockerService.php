<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Context;
use Aegir\Provision\Robo\ProvisionExecutor;
use Aegir\Provision\Robo\ProvisionTasks;
use Aegir\Provision\Robo\Task\Log;
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
   * @var string The name of this server's container.
   */
  private $containerName;


  /**
   * @var string The tag for this server's container.
   */
  private $containerTag;

  /**
   * HttpApacheDockerService constructor.
   * @param $service_config
   * @param Context $provider_context
   */
  function __construct($service_config, Context $provider_context)
  {
      parent::__construct($service_config, $provider_context);

      $this->containerName = "provision_http_{$this->provider->name}";
      $this->containerTag = "provision/http:{$this->provider->name}";
  }

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
  
  public function verifyServer() {
      $tasks = [];

      $provision = $this->getProvision();
      $robo = $this->getProvision()->getTasks();
      $provider = $this->provider;
      $build_dir = __DIR__ . DIRECTORY_SEPARATOR . '/ApacheDocker/';
      
      // Write Apache configuration files.
      $tasks['Saved Apache configuration files.'] = function () use ($provision) {
          $this->writeConfigurations();
      };
      
      // Build Docker image.
      $tasks['http.docker.build'] = $this->getProvision()->newTask()
          ->success('Built new Docker image for Apache: ' . $this->containerTag)
          ->failure('Unable to build docker container with tag: ' . $this->containerTag)
          ->execute(function () use ($provision, $build_dir) {
          $this->getProvision()->getTasks()->taskDockerBuild($build_dir)
              ->tag($this->containerTag)
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
      });
      
      // Docker run
      $tasks['Run docker image.'] = function () use ($provision,$provider) {
        
          // Check for existing container.
          $containerExists = $provision->getTasks()
              ->taskExec("docker ps -a -q -f name={$this->containerName}")
              ->silent(!$this->getProvision()->getOutput()->isVerbose())
              ->printOutput(false)
              ->storeState('container_id')
              ->taskFileSystemStack()
              ->defer(
                  function ($task, $state) use ($provision, $provider) {
                      $container = $state['container_id'];
                      if ($container) {
                        
                          //Check that it is running
                          $provision->getTasks()
                              ->taskExec("docker inspect -f '{{.State.Running}}' {$this->containerName}")
                              ->silent(!$provision->getOutput()->isVerbose())
                              ->printOutput(false)
                              ->storeState('running')
                              ->taskFileSystemStack()
                              ->defer(
                                  function ($task, $state) use ($provision, $provider) {
                                    
                                      if ($state['running'] == 'true') {
                                          $provision->io()->successLite('Container is already running: ' . $this->containerName);
                                      }
                                      // If not running, try to start it.
                                      else {
                                          $startResult = $provision->getTasks()
                                              ->taskExec("docker start {$this->containerName}")
                                              ->silent(!$provision->getOutput()->isVerbose())
                                              ->run()
                                          ;
                                        
                                          if ($startResult->wasSuccessful()) {
                                              $provision->io()->successLite('Existing container found. Restarted container ' . $this->containerName);
                                          }
                                          else {
                                              $provision->io()->errorLite('Unable to restart docker container: ' . $this->containerName);
                                              throw new \Exception('Unable to restart docker container: ' . $this->containerName);
                                          }
                                      }
                                  })
                              ->run();
                        
                      }
                    
                      # Docker container not found. Start it.
                      else {
                          $configVolumeHost = $provision->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $this->provider->name;
                          $configVolumeGuest = $this->provider->getProperty('aegir_root') . '/config/' . $this->provider->name;
                        
                          $result = $provision->getTasks()->taskDockerRun($this->containerTag)
                              ->detached()
                              ->publish($this->getProperty('http_port'), 80)
                              ->name($this->containerName)
                              ->volume($configVolumeHost, $configVolumeGuest)
                              ->silent(!$provision->getOutput()->isVerbose())
                              ->interactive()
                              ->run();
                        
                          if ($result->wasSuccessful()) {
                              $provision->io()->successLite('Running Docker image ' . $this->containerTag);
                          }
                          else {
                              $provision->io()->errorLite('Unable to run docker container: ' . $this->containerName);
                              throw new \Exception('Unable to run docker container: ' . $this->containerName);
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
      };
      
      $tasks['http.restart'] = $this->getProvision()->newTask()
          ->execute(function() {
              $this->restartService();
          })
          ->success('Restarted web service.')
          ->failure('Unable to restart web service.');
      
      return $tasks;
  }

  /**
   * Override HttpService by adding 'docker exec' in front of it.
   */
  public function restartService() {
      $this->properties['restart_command'] = "docker exec {$this->containerName}  {$this->properties['restart_command']}";
      parent::restartService();
  }
//
//  public function verify2()
//  {
//      $provision = $this->getProvision();
//      $provider = $this->provider;
//      $logger = $this->getProvision()->getLogger();
//      $collection = $this->getProvision()->getBuilder();
//      $collection->addCode(
//          function () use ($logger) {
//              $this->writeConfigurations();
//          }
//      );
//
//      $tag = "provision/http:{$this->provider->name}";
//      $name = "provision_http_{$this->provider->name}";
//      $build_dir = __DIR__ . DIRECTORY_SEPARATOR . '/ApacheDocker/';
//
//      # Build Docker Image
//      $collection->getCollection()->addCode(function () use ($provision, $tag, $name, $provider, $build_dir) {
//          $buildResult = $this->getProvision()->getTasks()->taskDockerBuild($build_dir)
//              ->tag($tag)
//              ->option(
//                  '-f',
//                  __DIR__.DIRECTORY_SEPARATOR.'/ApacheDocker/http.Dockerfile'
//              )
//              ->option(
//                  '--build-arg',
//                  "AEGIR_SERVER_NAME={$this->provider->name}"
//              )
//              ->option('--build-arg', "AEGIR_UID=".posix_getuid())
//              ->silent(!$this->getProvision()->getOutput()->isVerbose())
//              ->run()
//          ;
//
//          if ($buildResult->wasSuccessful()) {
//              $provision->io()->successLite('Built new Docker image for Apache: ' . $tag);
//          }
//          else {
//              $provision->io()->errorLite('Unable to build docker container with tag: ' . $tag);
//              throw new \Exception('Unable to build docker container with tag: ' . $tag);
//          }
//      }, 'docker-build');
//
//      # Run Docker Image
//      $collection->getCollection()->addCode(function () use ($provision, $tag, $name, $provider) {
//
//          // Check for existing container.
//          $containerExists = $provision->getTasks()
//              ->taskExec("docker ps -a -q -f name={$name}")
//                    ->silent(!$this->getProvision()->getOutput()->isVerbose())
//                  ->printOutput(false)
//                  ->storeState('container_id')
//              ->taskFileSystemStack()
//                  ->defer(
//                      function ($task, $state) use ($provision, $tag, $name, $provider) {
//                          $container = $state['container_id'];
//                          if ($container) {
//
//                              //Check that it is running
//                              $provision->getTasks()
//                                  ->taskExec("docker inspect -f '{{.State.Running}}' {$name}")
//                                      ->silent(!$provision->getOutput()->isVerbose())
//                                      ->printOutput(false)
//                                      ->storeState('running')
//                                  ->taskFileSystemStack()
//                                      ->defer(
//                                          function ($task, $state) use ($provision, $tag, $name, $provider) {
//
//                                              if ($state['running'] == 'true') {
//                                                  $provision->io()->successLite('Container is already running: ' . $name);
//                                              }
//                                              // If not running, try to start it.
//                                              else {
//                                                  $startResult = $provision->getTasks()
//                                                      ->taskExec("docker start {$name}")
//                                                      ->silent(!$provision->getOutput()->isVerbose())
//                                                      ->run()
//                                                  ;
//
//                                                  if ($startResult->wasSuccessful()) {
//                                                      $provision->io()->successLite('Existing container found. Restarted container ' . $name);
//                                                  }
//                                                  else {
//                                                      $provision->io()->errorLite('Unable to restart docker container: ' . $name);
//                                                      throw new \Exception('Unable to restart docker container: ' . $name);
//                                                  }
//                                              }
//                                          })
//                                  ->run();
//
//                          }
//
//                          # Docker container not found. Start it.
//                          else {
//                              $configVolumeHost = $provision->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $this->provider->name . DIRECTORY_SEPARATOR . '/apache';
//                              $configVolumeGuest = $this->provider->getProperty('aegir_root') . '/config/' . $this->provider->name . DIRECTORY_SEPARATOR . '/apache';
//
//                              $result = $provision->getTasks()->taskDockerRun($tag)
//                                  ->detached()
//                                  ->publish(80)
//                                  ->name($name)
//                                  ->volume($configVolumeHost, $configVolumeGuest)
//                                  ->silent(!$provision->getOutput()->isVerbose())
//                                  ->interactive()
//                                  ->run();
//
//                              if ($result->wasSuccessful()) {
//                                  $provision->io()->successLite('Running Docker image ' . $tag);
//                              }
//                              else {
//                                  $provision->io()->errorLite('Unable to run docker container: ' . $name);
//                                  throw new \Exception('Unable to run docker container: ' . $name);
//                              }
//                          }
//                      }
//                  )
//              ->run();
//          if ($containerExists->wasSuccessful()) {
//              $this->getProvision()->getLogger()->info('All processes completed successfully.');
//          }
//          else {
//              throw new \Exception('Something went wrong.');
//          }
//      });
//
//      $collection->getCollection()->addCode(function () use ($provision, $tag, $name, $provider) {
          
//          $this->getProvision()->application->console->runCollection($);
//          $provision->io()->customLite('Waiting for godot...', '𝙋');
//          $provision->io()->progressStart();
//
//
//          sleep(1);
//          $provision->io()->progressAdvance();
//
//          sleep(1);
//          $provision->io()->progressAdvance();
//
//          sleep(1);
//          $provision->io()->progressAdvance();
//
//          sleep(1);
//          $provision->io()->progressAdvance();
//
//          sleep(1);
//          $provision->io()->progressAdvance();
//          sleep(1);
//
//          $provision->io()->progressFinish();
//
    
//      });
//      return $collection;
//  }
}

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
      $collection->getCollection()->add(
          $this->getProvision()->getTasks()->taskDockerBuild($build_dir)
              ->tag($tag)
              ->option('-f', __DIR__ . DIRECTORY_SEPARATOR . '/ApacheDocker/http.Dockerfile')
              ->option('--build-arg', "AEGIR_SERVER_NAME={$this->provider->name}")
              ->option('--build-arg', "AEGIR_UID=" . posix_getuid())
              ->silent(!$this->getProvision()->getOutput()->isVerbose())
                , 'docker-build'
      );
      
      $collection->getCollection()->after('docker-build', function () use ($provision, $tag) {
          $provision->io()->successLite('Built new Docker image for Apache: ' . $tag);
      });

      # Run Docker Image
      $configVolumeHost = $this->getProvision()->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $this->provider->name . DIRECTORY_SEPARATOR . '/apache';
      $configVolumeGuest = $this->provider->getProperty('aegir_root') . '/config/' . $this->provider->name . DIRECTORY_SEPARATOR . '/apache';

      $collection->getCollection()->add(
          $this->getProvision()->getTasks()->taskDockerRun($tag)
              ->detached()
              ->publish(80)
              ->name($name)
              ->volume($configVolumeHost, $configVolumeGuest)
              ->silent(!$this->getProvision()->getOutput()->isVerbose())
              ->interactive()
              , 'docker-run')
      ;
      $collection->getCollection()->after('docker-run', function () use ($provision, $tag) {
          $provision->io()->successLite('Running Docker image ' . $tag);
      });
      
      return $collection;
  }
}

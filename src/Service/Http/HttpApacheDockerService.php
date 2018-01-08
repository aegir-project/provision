<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Configuration;
use Aegir\Provision\Context;
use Aegir\Provision\Provision;
use Aegir\Provision\Robo\ProvisionExecutor;
use Aegir\Provision\Robo\ProvisionTasks;
use Aegir\Provision\Robo\Task\Log;
use Aegir\Provision\Service\DockerServiceInterface;
use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfiguration;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfiguration;
use Aegir\Provision\Service\Http\ApacheDocker\Configuration\ServerConfiguration;
use Behat\Mink\Exception\Exception;
use Psr\Log\LogLevel;
use Robo\Task\Base\Exec;
use Robo\Task\Docker\Run;
use Robo\Tasks;
use Symfony\Component\Yaml\Yaml;

/**
 * Class HttpApacheDockerService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpApacheDockerService extends HttpApacheService implements DockerServiceInterface
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

    /**
     * Alter the root path because Provision stores the local path.
     *
     * Apache config must reflect the path inside the container.
     *
     * @param Configuration $config
     */
  public function processConfiguration(Configuration &$config) {

      // Replace platform's stored root with server's root.
      if ($this->context instanceof Context\SiteContext) {
          $root_on_host = $this->context->platform->getProperty('root');
      }
      elseif ($this->context instanceof Context\PlatformContext) {
          $root_on_host = $this->context->getProperty('root');
      }
      else {
          return;
      }

      $config->data['root'] = $this->mapContainerPath($root_on_host);

      if ($this->context instanceof Context\SiteContext) {
          $config->data['site_path'] = $config->data['root'] . '/sites/' . $config->data['uri'];
      }
      
      // When running in docker, internal port is always 80.
      $config->data['http_port'] = 80;
  }
    
    /**
     * Convert a path on the host like /home/jon/hostmaster to /var/aegir/hostmaster
     *
     * @param $root_on_host
     *
     * @return string
     */
    function mapContainerPath($root_on_host) {
        $path_parts = explode(DIRECTORY_SEPARATOR, $root_on_host);
        $directory = array_pop($path_parts);
        return $this->provider->getProperty('aegir_root') . DIRECTORY_SEPARATOR . 'platforms' . DIRECTORY_SEPARATOR . $directory;
    }

    /**
     * Run when a server is verified.
     *
     * @return array
     */
  public function verifyServer() {
      $tasks = [];
      $provision = $this->getProvision();

      $is_docker_server = FALSE;
      $compose_services = [];
      $filename = $this->provider->getProperty('server_config_path') . DIRECTORY_SEPARATOR . 'docker-compose.yml';

      // Load docker compose data from each docker service.
      $this->provider->getServices();
      foreach ($this->provider->getServices() as $type => $service) {
          if ($service instanceof DockerServiceInterface) {
              $compose_services[$type] = $service->dockerComposeService();
              $compose_services[$type]['hostname'] = $this->provider->name . '.' . $type;
          }
      }

      // If there are any docker services in this server create a
      // docker-compose file.
      $compose = array(
          'version' => '2',
          'services' => $compose_services,
      );

      // Write Apache configuration files.
      $tasks['http.configuration'] = Provision::newTask()
              ->start('Writing web server configuration...')
              ->execute(function() {
                  return $this->writeConfigurations()? 0: 1;
              });

      // Write docker-compose.yml file.
      $tasks['docker.compose.write'] = Provision::newTask()
          ->start('Generating docker-compose.yml file...')
          ->success('Generating docker-compose.yml file... Saved to ' . $filename)
          ->failure('Generating docker-compose.yml file... Saved to ' . $filename)
          ->execute(function () use ($compose, $filename) {

              $filename = $this->provider->getProperty('server_config_path') . DIRECTORY_SEPARATOR . 'docker-compose.yml';
              $server_name = $this->provider->name;
              $yml_prefix = <<<YML
# Provision Docker Compose File
# =============================
# Server: $server_name
#
# $filename
# 
# DO NOT EDIT THIS FILE.
# This file was automatically generated by Provision CLI.
#
# To re-generate this file, run the command:
#
#    provision verify $server_name
#
# Soon there will be an easy way for you to modify this file automatically.
# THANKS!

YML;
              $yml_dump = $yml_prefix . Yaml::dump($compose, 5, 2);
              $debug_message = 'Generated Docker Compose file: ' . PHP_EOL . $yml_dump;
              $this->getProvision()->getTasks()->taskLog($debug_message, LogLevel::INFO)->run()->getExitCode();

              $this->provider->fs->dumpFile($filename, $yml_dump);
          });

      // Run docker-compose up -d --build
      $tasks['docker.compose.up'] = Provision::newTask()
          ->start("Running <info>docker-compose up -d</info> in <info>{$this->provider->server_config_path}</info> ...")
          ->execute(function() {

              return Provision::getProvision()->getTasks()->taskExec('docker-compose')
                  ->dir($this->provider->server_config_path)
                  ->arg('up')
                  ->arg('-d')
                  ->arg('--build')
                  ->silent(!$this->getProvision()->getOutput()->isVerbose())
                  ->run()
                  ->getExitCode()
                  ;
          })
      ;

      return $tasks;
  }

    /**
     * Return the docker image name to use for this service.
     *
     * @return string
     */
    public function dockerImage()
    {
        return 'aegir/web:' . $this->provider->name;
    }

    public function dockerComposeService(){
        return [
            'image'  => $this->dockerImage(),
            'build' => [
                'context' => __DIR__ . DIRECTORY_SEPARATOR . 'ApacheDocker',
                'dockerfile' => 'http.Dockerfile',
                'args' => [
                    "AEGIR_UID" => $this->getProvision()->getConfig()->get('aegir_uid'),
                    "AEGIR_SERVER_NAME" => $this->provider->name,
                ],
            ],
            'restart'  => 'always',
            'ports'  => array(
                $this->getProperty('http_port') . ':80',
            ),
            'volumes' => $this->getVolumes(),
            'environment' => $this->getEnvironment()
        ];
    }


    /**
     * Return all volumes for this server.
     *
     * @TODO: Invoke an alter hook of some kinds to allow additional volumes and volume flags.
     *
     * To allow Aegir inside a container to properly launch other containers with mapped volumes, set an environment variable on your aegir/hostmaster container:
     *
     *   HOST_AEGIR_HOME=/home/you/Projects/aegir/aegir-home
     *
     * @return array
     */
    function getVolumes() {
        $volumes = array();

        $config_path_host = $config_path_container = $this->provider->getProperty('server_config_path');
        $volumes[] = "{$config_path_host}:/var/aegir/config/{$this->provider->name}:z";

//        $platforms_path_host = $platforms_path_container = d()->http_platforms_path;
//
//        if (isset($_SERVER['HOST_AEGIR_HOME'])) {
//            $config_path_host = strtr($config_path_host, array(
//                '/var/aegir' => $_SERVER['HOST_AEGIR_HOME']
//            ));
//            $platforms_path_host = strtr($platforms_path_host, array(
//                '/var/
//aegir' => $_SERVER['HOST_AEGIR_HOME']
//            ));
//        }
//
//
//        if (!empty($platforms_path_host) && !empty($platforms_path_container)) {
//            $volumes[] = "{$platforms_path_host}:{$platforms_path_container}:z";
//        }

        return $volumes;
    }

    /**
     * Load environment variables for this server.
     * @return array
     */
    function getEnvironment() {
        $environment = array();
        $environment['AEGIR_SERVER_NAME'] = $this->getContext()->name;
        return $environment;
    }
}

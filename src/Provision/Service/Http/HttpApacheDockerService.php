<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\ConfigFile;
use Aegir\Provision\Context;
use Aegir\Provision\Provision;
use Aegir\Provision\Service\DockerServiceInterface;
use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfigFile;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfigFile;
use Aegir\Provision\Service\Http\ApacheDocker\Configuration\ServerConfigFile;
use Aegir\Provision\ServiceSubscriber;
use Psr\Log\LogLevel;
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

      $this->setProperty('restart_command', $this->default_restart_cmd());
      $this->setProperty('web_group', $this->default_web_group());
  }

    /**
     * The web restart command is fixed to our service because we have the Dockerfile and build the container.
     *
     * @return string
     */
    public static function default_restart_cmd() {
//        return 'docker-compose exec http sudo apache2ctl graceful';

        // @TODO: restarting apache gracefully results in zero downtime, but we need to restart the
        // container to ensure volumes are mounted properly. If the root folder of the platform is deleted,
        // docker will not see a new folder in that path in it's place. The container must restart to
        // see the volume path.

        // @TODO: docker-compose restart doesn't catch errors in the apache config! Another win for restarting apache, not the entire container.
        return 'docker-compose exec http sudo apache2ctl graceful';
    }

    /**
     * Return the name of the apache user group for the webserver.
     * Docker is under our control, so this is not user configurable.
     * @return string
     */
    public static function default_web_group() {
        return 'www-data';
    }



    /**
     * Implements Service::server_options()
     *
     * @return array
     */
    static function server_options()
    {
        return [
            'http_port' => Provision::newProperty()
                ->description('The port which the web service is running on.')
                ->defaultValue(80)
                ->required()
            ,
        ];
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
    $configs['server'][] = ServerConfigFile::class;
    $configs['platform'][] = PlatformConfigFile::class;

    // Make sure to write platform and site config when verifying site.
    $configs['site'][] = PlatformConfigFile::class;
    $configs['site'][] = SiteConfigFile::class;
    return $configs;
  }

    /**
     * Alter the root path because Provision stores the local path.
     *
     * Apache config must reflect the path inside the container.
     *
     * @param ConfigFile $config
     */
  public function processConfiguration(ConfigFile &$config) {

      // Replace platform's stored root with server's root.
      if ($this->context instanceof Context\SiteContext || $this->context instanceof Context\PlatformContext) {
          $root_on_host = $this->context->getProperty('root');
      }
      else {
          return;
      }

      // Recalculate document root full inside container. Don't map container path with docroot.
      if ($this->context->getProperty('document_root')) {
          $config->data['document_root_full'] = $this->mapContainerPath($root_on_host) . DIRECTORY_SEPARATOR . $this->context->getProperty('document_root');
      }
      else {
          $config->data['document_root_full'] = $this->mapContainerPath($root_on_host);
      }

      if ($this->context->type == 'site' && isset($config->data['uri'])) {
          $config->data['site_path'] = $config->data['document_root'] . '/sites/' . $config->data['uri'];
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
    function mapContainerPath($root_on_host, $docroot = '') {


        $path_parts = explode(DIRECTORY_SEPARATOR, $root_on_host);
        $directory = array_pop($path_parts);
        return '/var/aegir/platforms/' . $directory;
    }

    /**
     * Run when a server is verified.
     *
     * @return array
     */
  public function verifyServer() {
      $tasks = [];

      // Write Apache configuration files.
      $tasks['http.server.configuration'] = Provision::newTask()
          ->start('Writing web server configuration...')
          ->execute(function() {
              return $this->writeConfigurations()? 0: 1;
          });

      return $tasks;
  }

    /**
     * React to the `provision verify` command on Site contexts
     */
    function verifySite() {
        $this->subscription = $this->getContext()->getSubscription('http');

        $server_tasks = $this->verifyServer();
        $platform_tasks = $this->verifyPlatform();

        $tasks = [];
        $tasks['http.site.configuration'] =  $this->getProvision()->newTask()
            ->start('Writing site web server configuration...')
            ->execute(function () {
                return $this->writeConfigurations($this->getContext())? 0: 1;
            })
        ;
        $tasks['http.platform.configuration'] = $platform_tasks['http.platform.configuration'];
        $tasks['http.server.configuration'] = $server_tasks['http.server.configuration'];

        $tasks['docker.compose.write'] = $server_tasks['docker.compose.write'];
        $tasks['docker.compose.up'] = $server_tasks['docker.compose.up'];
        $tasks['docker.http.restart'] = $server_tasks['docker.http.restart'];

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
                    "AEGIR_UID" => $this->getProvision()->getConfig()->get('script_uid'),
                    "APACHE_UID" => $this->getProvision()->getConfig()->get('web_user_uid'),
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

        // Map a volume for every site.
        $contexts = $this->getProvision()->getAllContexts();
        foreach ($contexts as $context) {
            if ($context instanceof ServiceSubscriber && $context->getSubscription('http')->server->name == $this->provider->name) {
                $container_path = $this->mapContainerPath($context->getProperty('root'));
                $volumes[$container_path] = $context->getProperty('root') . ':' . $container_path . ':z';
            }
        }

        return array_values($volumes);
    }

    /**
     * Load environment variables for this server.
     * @return array
     */
    function getEnvironment() {
        $environment = array();
        $environment['AEGIR_SERVER_NAME'] = $this->provider->name;
        return $environment;
    }
}

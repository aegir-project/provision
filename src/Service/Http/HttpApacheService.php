<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfiguration;
use Aegir\Provision\Service\Http\Apache\Configuration\ServerConfiguration;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfiguration;
use Aegir\Provision\Service\HttpService;

/**
 * Class HttpApacheService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpApacheService extends HttpService
{
  const SERVICE_TYPE = 'apache';
  const SERVICE_TYPE_NAME = 'Apache';
  
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
     * Determine apache restart command based on available executables.
     * @return string
     */
    static function default_restart_cmd() {
        $command = self::getApacheExecutable();
        return "sudo $command graceful";
    }

    /**
     * Find the nginx executable and return the path to it.
     *
     * @return mixed|string
     */
    public static function getApacheExecutable() {
        $command = '/usr/sbin/apachectl'; // A proper default for most of the world
        foreach (explode(':', $_SERVER['PATH']) as $path) {
            $options[] = "$path/apache2ctl";
            $options[] = "$path/apachectl";
        }
        // Try to detect the apache restart command.
        $options[] = '/usr/local/sbin/apachectl'; // freebsd
        $options[] = '/usr/sbin/apache2ctl'; // debian + apache2
        $options[] = '/usr/apache2/2.2/bin'; // Solaris
        $options[] = $command;

        foreach ($options as $test) {
            if (is_executable($test)) {
                return $command;
            }
        }
    }

    /**
     * React to `provision verify` command when run on a subscriber, to verify the service's provider.
     *
     * This is used to allow skipping of the service restart.
     */
    function verifyServer()
    {
        $tasks['http.configuration'] = $this->getProvision()->newTask()
            ->start('Writing web server configuration...')
            ->execute(function() {
                return $this->writeConfigurations()? 0: 1;
            })
        ;
        $tasks['http.config_link'] = $this->getProvision()->newTask()
            ->start('Checking for apache configuration link...')
->execute(function() {
                $provision_apache_config_path = $this->provider->server_config_path . DIRECTORY_SEPARATOR . $this->getType() . '.conf';
                $output = $this->provider->shell_exec( self::getApacheExecutable() . ' -S');

                if (strpos($output, 'PROVISION_VERSION') !== FALSE) {
                    return 0;
                }
                else {
                    $path = '/path/to/apache2/conf.d';

                    // Try to detect the apache restart command.
                    $options[] = '/private/etc/apache2/other';
                    $options[] = '/etc/apache/conf.d';
                    $options[] = '/etc/apache2/conf.d';
                    $options[] = '/etc/httpd/conf.d';

                    foreach ($options as $test) {
                        if (is_dir($test)) {
                            $path = $test;
                            break;
                        }
                    }

                    throw new \Exception("Provision configuration is not being loaded by apache. \n \nRun the command: sudo ln -s {$provision_apache_config_path} {$path}/provision.conf");
                }
          });

        $tasks['http.restart'] = $this->getProvision()->newTask()
            ->start('Restarting web server...')
            ->execute(function() {
                return $this->restartService()? 0: 1;
            })
        ;
        return $tasks;
    }
}

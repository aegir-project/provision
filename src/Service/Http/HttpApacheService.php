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


    static function default_restart_cmd() {
        return self::apache_restart_cmd();
    }

    /**
     * Guess at the likely value of the http_restart_cmd.
     *
     * This method is a static so that it can be re-used by the apache_ssl
     * service, even though it does not inherit this class.
     */
    public static function apache_restart_cmd() {
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
                $command = $test;
                break;
            }
        }

        return "sudo $command graceful";
    }
}

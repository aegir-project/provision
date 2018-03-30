<?php
/**
 * @file
 * The Provision HttpNginxService class.
 *
 * @see \Provision_Service_http_Nginx
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Provision;
use Aegir\Provision\Service\Http\Nginx\Configuration\ServerConfigFile;
use Aegir\Provision\Service\Http\Nginx\Configuration\SiteConfigFile;
use Aegir\Provision\Service\HttpService;

/**
 * Class HttpNginxService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpNginxService extends HttpService
{
    const SERVICE_TYPE = 'nginx';
    const SERVICE_TYPE_NAME = 'NGINX';


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
        $configs['site'][] = SiteConfigFile::class;
        return $configs;
    }

    /**
     * Determine nginx restart command based on available executables.
     * @return string
     */
    static function default_restart_cmd() {
        $command = self::getNginxExecutable();

        // For all commands other than the init.d service, add -s.
        if ($command != '/etc/init.d/nginx') {
            $command .= ' -s';
        }
        return "sudo $command reload";
    }

    /**
     * Implements Service::server_options()
     *
     * @return array
     */
    static function server_options()
    {
        $options = parent::server_options();
        $options['restart_command']->defaultValue(function () {
            return self::default_restart_cmd();
        });
        return $options;
    }
    /**
     * Guess at the likely value of the http_restart_cmd.
     *
     * This method is a static so that it can be re-used by the nginx_ssl
     * service, even though it does not inherit this class.
     */
    public static function getNginxExecutable() {
        $command = '/etc/init.d/nginx'; // A proper default for most of the world
        $options[] = $command;
        // Try to detect the nginx restart command.
        foreach (explode(':', $_SERVER['PATH']) as $path) {
            $options[] = "$path/nginx";
        }
        $options[] = '/usr/sbin/nginx';
        $options[] = '/usr/local/sbin/nginx';
        $options[] = '/usr/local/bin/nginx';

        foreach ($options as $test) {
            if (is_executable($test)) {
                return $test;
            }
        }

    }

}

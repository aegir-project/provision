<?php
/**
 * @file
 * The Provision HttpNginxService class.
 *
 * @see \Provision_Service_http_Nginx
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Service\Http\Nginx\Configuration\PlatformConfiguration;
use Aegir\Provision\Service\Http\Nginx\Configuration\ServerConfiguration;
use Aegir\Provision\Service\Http\Nginx\Configuration\SiteConfiguration;
use Aegir\Provision\Service\HttpService;

/**
 * Class HttpNginxService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpNginxService extends HttpService {

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
        $configs['server'][] = ServerConfiguration::class;
        $configs['site'][] = SiteConfiguration::class;
        return $configs;
    }

    /**
     * Determine nginx restart command based on available executables.
     * @return string
     */
    public static function default_restart_cmd() {
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
                $command = ($test == '/etc/init.d/nginx') ? $test : $test . ' -s';
                break;
            }
        }

        return "sudo $command reload";
    }
}

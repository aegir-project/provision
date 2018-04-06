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
        $options['php_fpm_sock_location'] = Provision::newProperty()
            ->description('Path to PHP FPM socket, or address and port that PHP-FPM is listening on (127.0.0.1:5000). NOTE: If installed using yum or apt, you may already have a configured "upstream" named "php-fpm", if so use "php-fpm" here.')
            ->defaultValue(self::getPhpFpmLocation())
            ->hidden()
        ;
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

    /**
     * Find the path to PHP FPM socket from common options.
     *
     * @return mixed
     */
    public static function getPhpFpmLocation() {
        $options[] = '/run/php-fpm/www.sock';
        $options[] = '/run/php/php7.2-fpm.sock';
        $options[] = '/run/php/php7.1-fpm.sock';
        $options[] = '/run/php/php7.0-fpm.sock';
        $options[] = '/var/run/php7-fpm.sock';
        $options[] = '/var/run/php5-fpm.sock';
        $options[] = '/var/run/php/php7.2-fpm.sock';
        $options[] = '/var/run/php/php7.1-fpm.sock';
        $options[] = '/var/run/php/php7.0-fpm.sock';
        $options[] = '/opt/remi/php72/root/tmp/php-fpm.sock';
        $options[] = '/opt/remi/php71/root/tmp/php-fpm.sock';
        $options[] = '/opt/remi/php70/root/tmp/php-fpm.sock';

        foreach ($options as $test) {
            if (Provision::fs()->exists($test)) {
                return 'unix:' . $test;
            }
        }

        return '127.0.0.1:5000';
    }
}

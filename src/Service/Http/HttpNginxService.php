<?php
/**
 * @file
 * The Provision HttpNginxService class.
 *
 * @see \Provision_Service_http_Nginx
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Provision;
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
     * Path to PHP FPM Socket file for php5.
     */
    const SOCKET_PATH_PHP5 = '/var/run/php5-fpm.sock';

    /**
     * Path to PHP FPM Socket file for php7.
     */
    const SOCKET_PATH_PHP7 = '/var/run/php/php7.0-fpm.sock';

    /**
     * Path to NGINX Control mode file.
     */
    const NGINX_CONTROL_MODE_FILE = '/etc/nginx/basic_nginx.conf';

    /**
     * HttpNginxService constructor.
     *
     * Detects NGINX configuration and sets as service properties.
     *
     * @param $service_config
     * @param \Aegir\Provision\Context\ServerContext $provider_context
     */
    function __construct($service_config, ServerContext $provider_context) {
        parent::__construct($service_config, $provider_context);

    }

    /**
     * Run when a `verify` command is invoked.
     *
     * @TODO: Should we move this to a different function? I don't want to store
     * these values in config files since they are read from the system.
     *
     * Are these values needed in other methods or commands? Is it ok for those
     * other methods to invoke verify() themselves if they need these properties?
     *
     * @return array
     */
    public function verify() {

        $nginx_config = $this->provider->shell_exec(self::getNginxExecutable() . ' -V');
        $this->setProperty('nginx_is_modern', preg_match("/nginx\/1\.((1\.(8|9|(1[0-9]+)))|((2|3|4|5|6|7|8|9|[1-9][0-9]+)\.))/", $nginx_config, $match));
        $this->setProperty('nginx_has_etag', preg_match("/nginx\/1\.([12][0-9]|[3]\.([12][0-9]|[3-9]))/", $nginx_config, $match));
        $this->setProperty('nginx_has_http2', preg_match("/http_v2_module/", $nginx_config, $match));
        $this->setProperty('nginx_has_upload_progress', preg_match("/upload/", $nginx_config, $match));
        $this->setProperty('nginx_has_gzip', preg_match("/http_gzip_static_module/", $nginx_config, $match));

        // Use basic nginx configuration if this control file exists.
        if (Provision::fs()->exists(self::NGINX_CONTROL_MODE_FILE)) {
            $this->setProperty('nginx_config_mode', 'basic');
            $this->getProvision()->getLogger()->info('Basic Nginx Config Active -SAVE- YES control file found {path}.', [
                'path' => self::NGINX_CONTROL_MODE_FILE,
            ]);
        }
        else {
            $this->setProperty('nginx_config_mode', 'extended');
            $this->getProvision()->getLogger()->info('Extended Nginx Config Active -SAVE- NO control file found {path}.', [
                'path' => self::NGINX_CONTROL_MODE_FILE,
            ]);
        }

        // Check if there is php-fpm listening on unix socket, otherwise use port 9000 to connect
        $this->setProperty('phpfpm_mode', self::getPhpFpmMode());

        // @TODO: Work out a way for something like BOA to do this via a plugin.
        // Check if there is BOA specific global.inc file to enable extra Nginx locations
//        if ($this->provider->fs->exists('/data/conf/global.inc')) {
//            $this->setProperty('satellite_mode', 'boa');
//            drush_log(dt('BOA mode detected -SAVE- YES file found @path.', array('@path' => '/data/conf/global.inc')));
//        }
//        else {
//            $this->server->satellite_mode = 'vanilla';
//            drush_log(dt('Vanilla mode detected -SAVE- NO file found @path.', array('@path' => '/data/conf/global.inc')));
//        }

        return parent::verify();
    }

    /**
     * No tasks to run
     * @return array
     */
    function verifyPlatform() {
        $tasks = [];
        return $tasks;
    }

    /**
    /**
     * Override restart_command task to use our default_restart_cmd()
     *
     * @TODO: Not sure why this is needed? Why does "self" not return this class's default_restart_cmd()?
     *
     * @return array
     */
    static function server_options() {
        $options = parent::server_options();
        $options['restart_command'] = Provision::newProperty()
            ->description('The command to reload the web server configuration.')
            ->defaultValue(function () {
                return self::default_restart_cmd();
            })
            ->required()
            ;
        return $options;
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
        $configs['site'][] = SiteConfiguration::class;
        return $configs;
    }

    /**
     * Determine full nginx restart command.
     *
     * @return string
     */
    public static function default_restart_cmd() {
        $command = self::getNginxExecutable();
        if ($command != '/etc/init.d/nginx') {
            $command .= ' -s';
        }

        return "sudo $command reload";
    }

    /**
     * Find the nginx executable and return the path to it.
     *
     * @return mixed|string
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
     * Determines the PHP FPM mode.
     *
     * @return string
     *   The mode, either 'socket' or 'port'.
     */
    public static function getPhpFpmMode() {

        // Search for socket files or fall back to port mode.
        switch (TRUE) {
            case Provision::fs()->exists(self::SOCKET_PATH_PHP5):
                $mode = 'socket';
                $socket_path = self::SOCKET_PATH_PHP5;
                break;
            case Provision::fs()->exists(self::SOCKET_PATH_PHP7):
                $mode = 'socket';
                $socket_path = self::SOCKET_PATH_PHP7;
                break;
            default:
                $mode = 'port';
                $socket_path = '';
                break;
        }

        // Return the discovered mode.
        return $mode;
    }
}

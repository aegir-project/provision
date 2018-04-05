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
use Aegir\Provision\Service\Http\Nginx\Configuration\SiteCommonConfiguration;
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
     *
     */
    const BOA_CONFIG_PATH = '/data/conf/global.inc';

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
        $this->setProperty('phpfpm_socket_path', self::getPhpFpmSocketPath());

        // @TODO: Work out a way for something like BOA to do this via a plugin.
        // Check if there is BOA specific global.inc file to enable extra Nginx locations
        if ($this->provider->fs->exists(self::BOA_CONFIG_PATH)) {
            $this->setProperty('satellite_mode', 'boa');
            $this->getProvision()->getLogger()->debug('BOA mode detected -SAVE- YES file found {path}.', ['path' => self::BOA_CONFIG_PATH]);
        }
        else {
            $this->setProperty('satellite_mode', 'vanilla');
            $this->getProvision()->getLogger()->debug('Vanilla mode detected -SAVE- NO file found {path}.', ['path' => self::BOA_CONFIG_PATH]);
        }

//        // Set correct subdirs_support value on server save
//        if (provision_hosting_feature_enabled('subdirs')) {
//            $this->server->subdirs_support = TRUE;
//        }

        $this->setProperty('server_config_path', $this->provider->server_config_path);

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
        $options['php_fpm_sock_location'] = Provision::newProperty()
            ->description('Path to PHP FPM socket, or address and port that PHP-FPM is listening on (127.0.0.1:5000). NOTE: If installed using yum or apt, you may already have a configured "upstream" named "php-fpm", if so use "php-fpm" here.')
            ->defaultValue(self::getPhpFpmLocation())
            ->hidden()
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
        $configs['server'][] = SiteCommonConfiguration::class;
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
     * @return array
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

    /**
     * Gets the PHP FPM unix socket path.
     *
     * If we're running in port mode, there is no socket path. FALSE would be
     * returned in this case.
     *
     * @return string
     *   The path, or FALSE if there isn't one.
     */
    public static function getPhpFpmSocketPath() {
        // Simply return FALSE if we're in port mode.
        if (self::getPhpFpmMode() == 'port') {
            return FALSE;
        }

        // Return the socket path based on the PHP version.
        if (strtok(phpversion(), '.') == 7) {
            return self::SOCKET_PATH_PHP7;
        }
        else {
            return self::SOCKET_PATH_PHP5;
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

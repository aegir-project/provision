<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfigFile;
use Aegir\Provision\Service\Http\Apache\Configuration\ServerConfigFile;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfigFile;
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
    $configs['server'][] = ServerConfigFile::class;
    $configs['platform'][] = PlatformConfigFile::class;
    $configs['site'][] = SiteConfigFile::class;
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
     * Determine apache command based on available executables.
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
     * Try to find the path to apache config.
     * @return mixed|string
     */
    public function getApacheConfPath() {

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
        return $path;
    }

    /**
     * Try to find the path to apache config.
     * @return mixed|string
     */
    public function getApacheConfFile() {

        $path = '';

        // Try to detect the apache restart command.
        // @TODO:
        $options[] = '/etc/apache2/httpd.conf';
        $options[] = '/etc/apache/apache.conf';
        $options[] = '/etc/apache2/apache2.conf';
        $options[] = '/etc/httpd/httpd.conf';

        foreach ($options as $test) {
            if (file_exists($test)) {
                $path = $test;
                break;
            }
        }
        return $path;
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
        $tasks['http.check'] = $this->getProvision()->newTask()
            ->start('Checking web server configuration...')
->execute(function() {
                $provision_apache_config_path = $this->provider->server_config_path . DIRECTORY_SEPARATOR . $this->getType() . '.conf';
                $error = NULL;

                // Catch any errors from apachectl call.
                try {
                    $output = $this->provider->shell_exec( self::getApacheExecutable() . ' -S');
                }
                catch (\Exception $e) {
                    $error = $e->getMessage();

                    // Check error to try and help the user.
                    // Make sure apache has mod_rewrite enabled.
                    if (strpos($error, 'mod_rewrite.so') !== FALSE) {
                        $conf_file = $this->getApacheConfFile();
                        $contents = file($conf_file);
                        foreach ($contents as $number => $line) {
                            if (strpos($line, 'rewrite_module') !== false) {
                                $line_string = $line;
                                $line_number = $number;
                                break;
                            }
                        }

                        $full_error = "The full error was: " . PHP_EOL . $error;

                        throw new \Exception("It doesn't appear that mod_rewrite is enabled in your system's Apache config. Please uncomment this line in {$conf_file} (Line $line_number):
                        
$line_string

$full_error
");
                    }
                    else {
                        throw new \Exception('EXCEPTION!!!' . $e->getMessage(), NULL, $e);
                    }
                }

                // Make sure Provision is writing apache config
                if (strpos($output, 'PROVISION_VERSION') === FALSE) {
                    $path = $this->getApacheConfPath();

                    throw new \Exception("Provision configuration is not being loaded by apache. \n \nRun the command: sudo ln -s {$provision_apache_config_path} {$path}/provision.conf");
                }

                // @TODO: Detect that mod_php is running too.
                // @TODO: Check that mbstring settings are set to PASS
    // MacOS default config does not support Drupal without changing this setting.
    // @TODO: Detect OpCache, help the user enable that.

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

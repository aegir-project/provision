<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Console\Config;
use Aegir\Provision\ContextProvider;
use Aegir\Provision\Property;
use Aegir\Provision\Provision;
use Aegir\Provision\Service\DockerServiceInterface;
use Psr\Log\LogLevel;
use Symfony\Component\Config\Definition\ConfigurationInterface;
use Symfony\Component\Filesystem\Filesystem;
use Symfony\Component\Yaml\Yaml;

/**
 * Class ServerContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_server
 */
class ServerContext extends ContextProvider implements ConfigurationInterface
{
    /**
     * @var string
     * 'server', 'platform', or 'site'.
     */
    public $type = 'server';
    const TYPE = 'server';

    public $server_config_path;

    /**
     * ServerContext constructor.
     *
     * Prepares "server_config_path" as the place to store this server's service
     * configuration files (apache configs, etc.).
     *
     * @param $name
     * @param Provision $provision
     * @param array $options
     */
    function __construct($name, Provision $provision, array $options = [])
    {
        // @TODO: Create a 'servers_path' to keep things nice and clean.
        parent::__construct($name, $provision, $options);
        $this->server_config_path = $this->getProvision()->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $name;
        $this->properties['server_config_path'] = $this->server_config_path;

        $this->fs = new Filesystem();
    }

    /**
     * @return string|Property[]
     */
    static function option_documentation()
    {
        return [
            'remote_host' =>
                Provision::newProperty()
                    ->description('server: host name')
                    ->required(TRUE)
                    ->defaultValue('localhost')
                    ->validate(function($remote_host) {
                        // If remote_host doesn't resolve to anything, warn the user.
                        $ip = gethostbynamel($remote_host);
                        if (empty($ip)) {
                            throw new \RuntimeException("Hostname $remote_host does not resolve to an IP address. Please try again.");
                        }
                        return $remote_host;
                  }),
            'script_user' =>
                Provision::newProperty()
                    ->description('server: OS user name')
                    ->required(TRUE)
                    ->defaultValue(Config::getScriptUser()),
            'aegir_root' =>
                Provision::newProperty()
                    ->description('server: aegir user home directory')
                    ->required(TRUE)
                    ->defaultValue(Config::getHomeDir()),
            // @TODO: Why do server contexts need a master_url?
            'master_url' =>
                Provision::newProperty()
                    ->description('server: Hostmaster URL')
                    ->required(FALSE)
        ];
    }

    /**
     * @return array
     */
    public function verify()
    {
        $tasks = [];
        return $tasks;
    }

    /**
     * Run a shell command on this server.
     *
     * @TODO: Run remote commands correctly.
     *
     * @param $cmd
     * @return string
     * @throws \Exception
     */
    public function shell_exec($cmd) {
        $output = '';
        $exit = 0;
        exec($cmd, $output, $exit);

        if ($exit != 0) {
            throw new \Exception("Command failed: $cmd");
        }

        return implode("\n", $output);
    }
}

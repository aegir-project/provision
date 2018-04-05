<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Console\Config;
use Aegir\Provision\ServiceProvider;
use Aegir\Provision\Property;
use Aegir\Provision\Provision;
use Aegir\Provision\Service\DockerServiceInterface;
use Psr\Log\LogLevel;
use Robo\ResultData;
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
class ServerContext extends ServiceProvider implements ConfigurationInterface
{
    /**
     * @var string
     * 'server', 'platform', or 'site'.
     */
    public $type = 'server';
    const TYPE = 'server';

    /**
     * @var string
     * The path to store the server's configuration files in.  ie. /var/aegir/config/server_master.
     */
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

        // If server_config_path property is empty, generate it from provision config_path + server name.
        if (empty($this->getProperty('server_config_path'))) {
            $this->server_config_path = $this->getProvision()->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $name;
            $this->setProperty('server_config_path', $this->server_config_path);
        }
        else {
            $this->server_config_path = $this->getProperty('server_config_path');
        }

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
//            // @TODO: Why do server contexts need a master_url?
//            'master_url' =>
//                Provision::newProperty()
//                    ->description('server: Hostmaster URL')
//                    ->required(FALSE),

            'server_config_path' =>
                Provision::newProperty()
                    ->description('server: The location to store the server\'s configuration files. If left empty, will be generated automatically.')
                    ->required(FALSE)
                    ->hidden()
            ,
        ];
    }

    /**
     * @return array
     */
    public function verify()
    {
        // Create the server/service directory. We put this here because we need to make sure this is always run before any other tasks.
        Provision::fs()->mkdir($this->server_config_path);

        $tasks = [];
        return $tasks;
    }

    /**
     * Run a shell command on this server.
     *
     * @param $cmd string The command to run
     * @param $dir string The directory to run the command in. Defaults to this server's config path.
     * @param $return string What to return. Can be 'output' or 'exit'.
     *
     * @return string
     * @throws \Exception
     */
    public function shell_exec($command, $dir = NULL, $return = 'output') {
        $cwd = getcwd();
        $original_command = $command;

        $tmpdir = sys_get_temp_dir() . '/provision';
        if (!Provision::fs()->exists($tmpdir)){
            Provision::fs()->mkdir($tmpdir);
        }

        $datestamp = date('c');
        $tmp_output_file = tempnam($tmpdir, 'task.' . $datestamp . '.output.');

        $effective_wd = $dir? $dir:
            $this->getProperty('server_config_path');

        if ($this->getProvision()->getOutput()->isVerbose()) {
            $this->getProvision()->io()->commandBlock($command, $effective_wd);
        }

        // Output and Errors to files.
        $command .= "> $tmp_output_file 2>&1";

        chdir($effective_wd);
        exec($command, $output, $exit);
        chdir($cwd);

        $output = file_get_contents($tmp_output_file);

        if (!empty($output)){
            if ($this->getProvision()->getOutput()->isVerbose()) {
                $this->getProvision()->io()->outputBlock($output);
            }
        }

        if ($exit != ResultData::EXITCODE_OK) {
            throw new \Exception($output);
        }

        return ${$return};
    }
}

<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Console\Config;
use Aegir\Provision\ContextProvider;
use Aegir\Provision\Property;
use Aegir\Provision\Provision;
use Symfony\Component\Config\Definition\ConfigurationInterface;

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
                    ->default('localhost')
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
                    ->default(Config::getScriptUser()),
            'aegir_root' =>
                Provision::newProperty()
                    ->description('server: aegir user home directory')
                    ->required(TRUE)
                    ->default(Config::getHomeDir()),
            // @TODO: Why do server contexts need a master_url?
            'master_url' =>
                Provision::newProperty()
                    ->description('server: Hostmaster URL')
                    ->required(FALSE)
        ];
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

<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Context;
use Symfony\Component\Config\Definition\Builder\TreeBuilder;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class ServerContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_server
 */
class ServerContext extends Context implements ConfigurationInterface
{
    /**
     * @var string
     * 'server', 'platform', or 'site'.
     */
    public $type = 'server';
    
    static function option_documentation()
    {
        $options = [
          'remote_host' => 'server: host name; default localhost',
          'script_user' => 'server: OS user name; default current user',
          'aegir_root' => 'server: Aegir root; default '.getenv('HOME'),
          'master_url' => 'server: Hostmaster URL',
        ];

        return $options;
    }

    public function verify() {

//        parent::verify();
        return "Server Context Verified: " . $this->name;
    }
}

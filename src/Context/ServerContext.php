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
    
    /**
     * {@inheritdoc}
     */
    public function getConfigTreeBuilder()
    {
        $tree_builder = new TreeBuilder();
        $root_node = $tree_builder->root('server');
        $root_node
            ->children()
                ->scalarNode('name')
                    ->defaultValue($this->name)
                ->end()
                ->scalarNode('remote_host')
                    ->defaultValue($this->properties['remote_host'])
                ->end()
                ->scalarNode('script_user')
                    ->defaultValue($this->properties['script_user'])
                ->end()
                ->scalarNode('aegir_root')
                    ->defaultValue($this->properties['aegir_root'])
                ->end()
                ->scalarNode('master_url')
                    ->defaultValue($this->properties['master_url'])
                ->end()
            ->end();
        
        return $tree_builder;
    }
    
}

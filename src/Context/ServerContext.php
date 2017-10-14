<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Context;
use Drupal\Console\Core\Style\DrupalStyle;
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

    /**
     * @var array
     * A list of services needed for this context.
     */
    protected $services = [];

    /**
     * ServerContext constructor.
     *
     * @param $name
     * @param $console_config
     * @param array $options
     */
    function __construct($name, $console_config, array $options = [])
    {
        parent::__construct($name, $console_config, $options);
        if (isset($this->config['services'])) {
            $this->services = $this->config['services'];
        }
    }

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
     * Return all services for this context.
     *
     * @return array
     */
    public function getServices() {
        return $this->services;
    }

    /**
     * {@inheritdoc}
     */
    public function configTreeBuilder(&$root_node)
    {
        $root_node
            ->children()
                ->arrayNode('services')
                    ->prototype('array')
                    ->children()
                        ->scalarNode('name')
                        ->isRequired(true)
                    ->end()
            ->end()
        ->end();
    }

    public function verify() {

//        parent::verify();
        return "Server Context Verified: " . $this->name;
    }

    /**
     * Output a list of all services for this service.
     */
    public function showServices(DrupalStyle $io) {
        if (!empty($this->getServices())) {
            $rows = [];
            foreach ($this->getServices() as $name => $service) {
                $rows[] = [$name, $service['name']];
            }
            $io->table(['Services'], $rows);
        }
    }
}

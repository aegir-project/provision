<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Application;
use Aegir\Provision\ContextSubscriber;
use Aegir\Provision\Provision;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class PlatformContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_platform
 */
class PlatformContext extends ContextSubscriber implements ConfigurationInterface
{
    /**
     * @var string
     */
    public $type = 'platform';
    const TYPE = 'platform';

    /**
     * @var \Aegir\Provision\Context\ServerContext;
     */
    public $web_server;

    /**
     * PlatformContext constructor.
     *
     * @param $name
     * @param Provision $provision
     * @param array $options
     */
    function __construct(
        $name,
        Provision $provision = NULL,
        $options = []
    ) {
        parent::__construct($name, $provision, $options);

        // Load "web_server" context.
        // There is no need to validate for $this->properties['web_server'] because the config system does that.
//        $this->web_server = $application->getContext($this->properties['web_server']);
    }
    
    static function option_documentation()
    {
        $options = [
          'root' => 'platform: path to a Drupal installation',
//          'server' => 'platform: drush backend server; default @server_master',

            // web_server will be loaded via another method. For now using configTreeBuilder()
//          'web_server' => 'platform: web server hosting the platform; default @server_master',
          'makefile' => 'platform: drush makefile to use for building the platform if it doesn\'t already exist',
          'make_working_copy' => 'platform: Specifiy TRUE to build the platform with the Drush make --working-copy option.',
        ];

        return $options;
    }

    /**
     * Platforms require a web (http) server.
     *
     * @return array
     */
    public static function serviceRequirements() {
        return ['http'];
    }
    
    /**
     * Output extra info before verifying.
     */
    public function verify()
    {
        $this->application->io->customLite($this->getProperty('root'), 'Root: ', 'info');
        $this->application->io->customLite($this->config_path, 'Configuration File: ', 'info');
        
        // This is how you can use Robo Tasks in a platform verification call.
        // The "silent" method actually works here.
        // It only partially works in Service::restartServices()!
        $this->getBuilder()->taskExec('env')
            ->silent(!$this->application->io->isVerbose())
            ->run();
        
        return parent::verify();
    }
}

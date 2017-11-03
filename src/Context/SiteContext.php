<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Application;
use Aegir\Provision\Context;
use Symfony\Component\Config\Definition\Builder\ArrayNodeDefinition;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class SiteContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_site
 */
class SiteContext extends Context implements ConfigurationInterface
{
    /**
     * @var string
     */
    public $type = 'site';

    /**
     * PlatformContext constructor.
     *
     * @param $name
     * @param $console_config
     * @param Application $application
     * @param array $options
     */
    function __construct($name, $console_config, Application $application, array $options = [])
    {
        parent::__construct($name, $console_config, $application, $options);

        // Load "web_server" and "platform" contexts.
        // There is no need to check if the property exists because the config system does that.
        $this->db_server = $application->getContext($this->properties['db_server']);
        $this->platform = $application->getContext($this->properties['platform']);
    }

    static function option_documentation()
    {
        return [
//          'platform' => 'site: the platform the site is run on',
//          'db_server' => 'site: the db server the site is run on',
          'uri' => 'site: example.com URI, no http:// or trailing /',
          'language' => 'site: site language; default en',
//          'aliases' => 'site: comma-separated URIs',
//          'redirection' => 'site: boolean for whether --aliases should redirect; default false',
//          'client_name' => 'site: machine name of the client that owns this site',
//          'install_method' => 'site: How to install the site; default profile. When set to "profile" the install profile will be run automatically. Otherwise, an empty database will be created. Additional modules may provide additional install_methods.',
          'profile' => 'site: Drupal profile to use; default standard',
//          'drush_aliases' => 'site: Comma-separated list of additional Drush aliases through which this site can be accessed.',
        ];
    }

    public static function serviceRequirements() {
        return ['db'];
    }

    /**
     * @TODO: Come up with another method to let Context nodes specify related contexts with ability to validate.
     * @param $root_node
     */
    function configTreeBuilder(ArrayNodeDefinition &$root_node) {
        $root_node
            ->children()
                ->setNodeClass('context', 'Aegir\Provision\ConfigDefinition\ContextNodeDefinition')
                ->node('db_server', 'context')
                    ->isRequired()
                    ->attribute('context_type', 'server')
                    ->attribute('service_requirement', 'db')
                ->end()
                ->node('platform', 'context')
                    ->isRequired()
                    ->attribute('context_type', 'platform')
                ->end()
        ->end();
    }

    public function verify() {
        parent::verify();

        // $this->db_server->service('db')->verify();
//        $this->platform->verify();

//        return "Site Context Verified: " . $this->name;
    }

    //  /**
    //   * Write out this named context to an alias file.
    //   */
    //  function write_alias() {
    //    $config = new Provision_Config_Drushrc_Alias($this->name, $this->properties);
    //    $config->write();
    //    foreach ($this->drush_aliases as $drush_alias) {
    //      $config = new Provision_Config_Drushrc_Alias($drush_alias, $this->properties);
    //      $config->write();
    //    }
    //  }
}

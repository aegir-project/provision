<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Application;
use Aegir\Provision\ContextSubscriber;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class SiteContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_site
 */
class SiteContext extends ContextSubscriber implements ConfigurationInterface
{
    /**
     * @var string
     */
    public $type = 'site';
    const TYPE = 'site';

    /**
     * @var \Aegir\Provision\Context\SiteContext
     */
    public $platform;


    /**
     * SiteContext constructor.
     *
     * @param $name
     * @param Application $application
     * @param array $options
     */
    function __construct($name, Application $application = NULL, array $options = [])
    {
        parent::__construct($name, $application, $options);

        // Load "web_server" and "platform" contexts.
        // There is no need to check if the property exists because the config system does that.
//        $this->db_server = $application->getContext($this->properties['db_server']);

        $this->platform = Application::getContext($this->properties['platform'], $application);


        // Add platform http service subscription.
        $this->serviceSubscriptions['http'] = $this->platform->getSubscription('http');

    }

    static function option_documentation()
    {
        return [
          'platform' => 'site: the platform the site is run on',
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
    
    public static function contextRequirements() {
        return [
            'platform' => 'platform'
        ];
    }

    /**
     * Overrides ContextSubscriber::getSubscriptions() to add in the platform's subscriptions.
     *
     * @return array
     */
    public function getSubscriptions() {

        // Load this context's subscriptions.
        $subscriptions = parent::getSubscriptions();

        // Load the platform's subscriptions.
        $subscriptions += $this->platform->getSubscriptions();

        return $subscriptions;
    }
}

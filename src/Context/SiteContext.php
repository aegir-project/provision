<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Context;
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

    static function option_documentation()
    {
        return [
          'platform' => 'site: the platform the site is run on',
          'db_server' => 'site: the db server the site is run on',
          'uri' => 'site: example.com URI, no http:// or trailing /',
          'language' => 'site: site language; default en',
          'aliases' => 'site: comma-separated URIs',
          'redirection' => 'site: boolean for whether --aliases should redirect; default false',
          'client_name' => 'site: machine name of the client that owns this site',
          'install_method' => 'site: How to install the site; default profile. When set to "profile" the install profile will be run automatically. Otherwise, an empty database will be created. Additional modules may provide additional install_methods.',
          'profile' => 'site: Drupal profile to use; default standard',
          'drush_aliases' => 'site: Comma-separated list of additional Drush aliases through which this site can be accessed.',
        ];
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

<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Application;
use Aegir\Provision\Context;
use Aegir\Provision\ContextSubscriber;
use Aegir\Provision\Provision;
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
     * @var \Aegir\Provision\Context\PlatformContext
     */
    public $platform;


    /**
     * SiteContext constructor.
     *
     * @param $name
     * @param Application $application
     * @param array $options
     */
    function __construct(
        $name,
        Provision $provision = NULL,
        $options = []
    ) {
        parent::__construct($name, $provision, $options);

        // Load "web_server" and "platform" contexts.
        // There is no need to check if the property exists because the config system does that.
//        $this->db_server = $application->getContext($this->properties['db_server']);

        // Load platform context... @TODO: Automatically do this for required contexts?
        $this->platform = $this->getProvision()->getContext($this->properties['platform']);
    
        // Add platform http service subscription.
        $this->serviceSubscriptions['http'] = $this->platform->getSubscription('http');
        $this->serviceSubscriptions['http']->setContext($this);
        
        $uri = $this->getProperty('uri');
        $this->properties['site_path'] = "sites/{$uri}";

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
     * Output extra info before verifying.
     */
    public function verify()
    {
        $this->getProvision()->io()->customLite($this->getProperty('uri'), 'Site URL: ', 'info');
        $this->getProvision()->io()->customLite($this->platform->getProperty('root'), 'Root: ', 'info');
        $this->getProvision()->io()->customLite($this->config_path, 'Configuration File: ', 'info');
        $this->getProvision()->io()->newLine();

    
        $tasks['site.prepare'] = $this->getProvision()->newTask()
            ->success('Prepared directories.')
            ->failure('Failed to prepare directories.')
            
            /**
             * There are many ways to do this...
             * This way is very verbose and I cannot figure out how to quiet it down.
             */
//            ->execute($this->getProvision()->getTasks()->taskFilesystemStack()
//                ->mkdir("$path/sites/$uri/files")
//                ->chmod("$path/sites/$uri/files", 02770)
//                ->chgrp("$path/sites/$uri/files", $this->getServices('http')->getProperty('web_group'))

                /**
                 * This way is quiet.
                 *
                 * @see verify.provision.inc
                 * @see drush_provision_drupal_pre_provision_verify()
                 */
                ->execute(function() {
                    $uri = $this->getProperty('uri');
                    $path = $this->platform->getProperty('root');
    
                // @TODO: These folders are how aegir works now. We might want to rethink what folders are created.
                    // Directories set to 755
                    $this->fs->mkdir([
                        "$path/sites/$uri",
                        
                    ], 0755);
        
                    // Directories set to 02775
                    $this->fs->mkdir([
                        "$path/sites/$uri/themes",
                        "$path/sites/$uri/modules",
                        "$path/sites/$uri/libraries",
                    ], 02775);
        
                    // Directories set to 02775
                    $this->fs->mkdir([
                        "$path/sites/$uri/files",
                        "$path/sites/$uri/files/tmp",
                        "$path/sites/$uri/files/images",
                        "$path/sites/$uri/files/pictures",
                        "$path/sites/$uri/files/css",
                        "$path/sites/$uri/files/js",
                        "$path/sites/$uri/files/ctools",
                        "$path/sites/$uri/files/imagecache",
                        "$path/sites/$uri/files/locations",
                        "$path/sites/$uri/files/styles",
                        "$path/sites/$uri/private",
                        "$path/sites/$uri/private/config",
                        "$path/sites/$uri/private/config/sync",
                        "$path/sites/$uri/private/files",
                        "$path/sites/$uri/private/temp",
                        "$path/sites/$uri/private/temp",
                    ], 02770);
                    
                    // Change certain folders to be in web server group.
                    $this->fs->chgrp([
                        "$path/sites/$uri/files",
                        "$path/sites/$uri/files/tmp",
                        "$path/sites/$uri/files/images",
                        "$path/sites/$uri/files/pictures",
                        "$path/sites/$uri/files/css",
                        "$path/sites/$uri/files/js",
                        "$path/sites/$uri/files/ctools",
                        "$path/sites/$uri/files/imagecache",
                        "$path/sites/$uri/files/locations",
                        "$path/sites/$uri/files/styles",
                        "$path/sites/$uri/private",
                        "$path/sites/$uri/private/config",
                        "$path/sites/$uri/private/config/sync",
                        "$path/sites/$uri/private/files",
                        "$path/sites/$uri/private/temp",
                        "$path/sites/$uri/private/temp",
                    ], $this->getSubscription('http')->service->getProperty('web_group'));
                });
    
        $tasks['site.settings'] = $this->getProvision()->newTask()
            ->success('Provision did not write your settings.php automatically. You must do this yourself now. Use `provision status {name}` to view the correct credentials.')
            ->failure('This wont fail, its a dummy placeholder.')
            ->execute(function () {
                return 0;
            })
            ;
        
        // FROM verify.provision.inc  drush_provision_drupal_pre_provision_verify() line 118
//        drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
//        // This is the actual drupal provisioning requirements.
//        _provision_drupal_create_directories();
//        _provision_drupal_maintain_aliases();
//        _provision_drupal_ensure_htaccess_update();
//        // Requires at least the database settings to complete.
//
//        _provision_drupal_create_settings_file();
//
//        // If this is the hostmaster site, save the ~/.drush/drushrc.php file.
//        if (d()->root == d('@hostmaster')->root && d()->uri == d('@hostmaster')->uri) {
//            $aegir_drushrc = new Provision_Config_Drushrc_Aegir();
//            $aegir_drushrc->write();
//        }
//
//        provision_drupal_push_site(drush_get_option('override_slave_authority', FALSE));
//
        return $tasks;
    }
    
    /**
     * Return a list of folders to create in the Drupal root.
     *
     * @TODO: Move this to the to-be-created DrupalPlatform class.
     */
    function siteFolders($uri = 'default') {
        return [
            "sites/$uri",
            "sites/$uri/files",
        ];
    }
}

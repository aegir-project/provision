<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Application;
use Aegir\Provision\ServiceSubscriber;
use Aegir\Provision\Provision;
use Symfony\Component\Config\Definition\ConfigurationInterface;

/**
 * Class SiteContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_site
 */
class SiteContext extends PlatformContext implements ConfigurationInterface
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
        if (!empty($this->properties['platform'])) {
            $this->platform = $this->getProvision()->getContext($this->properties['platform']);
        }
        else {
            $this->platform = NULL;
        }

        // Load http service from platform if site doesn't have them.
        if (!$this->hasService('http') && $this->platform->hasService('http')) {
            $this->serviceSubscriptions['http'] = $this->platform->getSubscription('http');
            $this->serviceSubscriptions['http']->setContext($this);
        }

        $uri = $this->getProperty('uri');
        $this->properties['site_path'] = "sites/{$uri}";

        // If site_path property is empty, generate it from platform root + uri.
        if (empty($this->getProperty('site_path'))) {
            $this->setProperty('site_path', $this->platform->getConfig()->get('root') . DIRECTORY_SEPARATOR . $this->uri);
        }
    }

    static function option_documentation()
    {
        $options = parent::option_documentation();

        $options['platform'] = Provision::newProperty()
            ->description('site: The platform this site is run on. (Optional)')
            ->required(FALSE)
        ;
        $options['uri'] = 'site: example.com URI, no http:// or trailing /';
        $options['language'] = 'site: site language; default en';
        $options['profile'] = 'site: Drupal profile to use; default standard';
        $options['site_path'] = Provision::newProperty()
            ->description('site: The site configuration path (sites/domain.com). If left empty, will be generated automatically.')
            ->required(FALSE)
        ;

        return $options;


//          'uri' => 'site: example.com URI, no http:// or trailing /',
//          'language' => 'site: site language; default en',
//          'aliases' => 'site: comma-separated URIs',
//          'redirection' => 'site: boolean for whether --aliases should redirect; default false',
//          'client_name' => 'site: machine name of the client that owns this site',
//          'install_method' => 'site: How to install the site; default profile. When set to "profile" the install profile will be run automatically. Otherwise, an empty database will be created. Additional modules may provide additional install_methods.',
//          'profile' => 'site: Drupal profile to use; default standard',
//          'drush_aliases' => 'site: Comma-separated list of additional Drush aliases through which this site can be accessed.',
//
//            'site_path' =>
//                Provision::newProperty()
//                    ->description('site: The site configuration path (sites/domain.com). If left empty, will be generated automatically.')
//                    ->required(FALSE)
//            ,
//
//        ];
    }

    public static function serviceRequirements() {
        $requirements[] = 'http';
        $requirements[] = 'db';
        return $requirements;
    }

    /**
     * Output extra info before verifying.
     */
    public function verify()
    {

        $tasks = parent::verify();

        $this->getProvision()->io()->customLite($this->getProperty('uri'), 'Site URL: ', 'info');
        $this->getProvision()->io()->customLite($this->getProperty('root'), 'Root: ', 'info');
        $this->getProvision()->io()->customLite($this->config_path, 'Configuration File: ', 'info');
        $this->getProvision()->io()->newLine();

    
        $tasks['site.prepare'] = $this->getProvision()->newTask()
            ->start('Preparing Drupal site configuration...')

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
                    $docroot = $this->getProperty('document_root_full');
                    $site_path = $docroot . DIRECTORY_SEPARATOR . $this->getProperty('site_path');

                // @TODO: These folders are how aegir works now. We might want to rethink what folders are created.
                    // Directories set to 755
                    $this->fs->mkdir("$site_path");
                    $this->fs->chmod($site_path, 0755);

                    // Directories set to 02775
                    $this->fs->mkdir([
                        "$site_path/themes",
                        "$site_path/modules",
                        "$site_path/libraries",
                    ]);
                    $this->fs->chmod([
                        "$site_path/themes",
                        "$site_path/modules",
                        "$site_path/libraries",
                    ], 02775);


                    // Directories set to 02775
                    $this->fs->mkdir([
                        "$site_path/files",
                    ]);
                    $this->fs->chmod([
                        "$site_path/files",
                    ], 02770);

                    // Change certain folders to be in web server group.
                // @TODO: chgrp only works when running locally with apache.
                // @TODO: Figure out a way to store host web group vs container web group, and get it working with docker web service.
                // @TODO: Might want to do chgrp verification inside container?

                    $dir = "$site_path/files";
                    $user = $this->getProvision()->getConfig()->get('web_user');
                    $this->getProvision()->getLogger()->info("Running chgrp {$dir} {$user}");
                    $this->fs->chgrp($dir, $user);

                    // Copy Drupal's default settings.php file into place.
                    if (!file_exists("$site_path/settings.php")) {
                        $this->fs->copy("$docroot/sites/default/default.settings.php", "$site_path/settings.php");
                    }

                    $this->fs->chmod("$site_path/settings.php", 02770);
                    $this->fs->chgrp("$site_path/settings.php", $this->getProvision()->getConfig()->get('web_user'));
            });


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

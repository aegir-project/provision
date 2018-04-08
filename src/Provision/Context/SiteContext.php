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
    }

    /**
     *
     */
    public function preSave() {
        if (empty($this->getProperty('site_path'))) {
            $this->setProperty('site_path', 'sites/' . DIRECTORY_SEPARATOR . $this->getProperty('uri'));
        }
    }

    static function option_documentation()
    {

        // @TODO: check for other sites with the URI.
        $options['uri'] = Provision::newProperty()
            ->description('site: example.com URI, no http:// or trailing /')
        ;

        $options['platform'] = Provision::newProperty()
            ->description('site: The platform this site is run on. (Optional)')
            ->required(FALSE)
        ;

        $options = array_merge($options, parent::option_documentation());

        $options['language'] = Provision::newProperty('site: site language; default en')
            //@TODO: Language handling across provision, and an arbitrary site install values tool.
            ->defaultValue('en')
        ;
        $options['profile'] = Provision::newProperty('site: Drupal profile to use; default standard')
            ->defaultValue('standard')
        ;
        $options['site_path'] = Provision::newProperty()
            ->description('site: The site configuration path (sites/domain.com). If left empty, will be generated automatically.')
            ->defaultValue('sites/default')
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

        // If a composer.json file is found, run composer install.
        if (Provision::fs()->exists($this->getProperty('root') . '/composer.json') && $composer_command = $this->getProperty('composer_install_command')) {
            $dir = $this->getProperty('root');
            $tasks['composer.install'] = $this->getProvision()->newTask()
                ->start("Running <comment>$composer_command</comment> in <comment>$dir</comment> ...")
                ->execute(function () use ($composer_command) {
                    return $this->shell_exec($composer_command, NULL, 'exit');
                });
        }

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


                    if (strpos(file_get_contents("$site_path/settings.php"), "// PROVISION SETTINGS") === FALSE) {

                        $crypt = $this->getProperty('root') . '/' . $this->getProperty('document_root') . '/core/lib/Drupal/Component/Utility/Crypt.php';
                        if (file_exists($crypt)) {
                            require_once $crypt;
                            $hash_salt = \Drupal\Component\Utility\Crypt::randomBytesBase64(55);
                        }
                        else {
                            $hash_salt = uniqid();
                        }

                        // @TODO: This is only true for Drupal version 7.50 and up. See Provision/Config/Drupal/Settings.php
                            // We are treading more and more into the Drupal-only world, so I'm leaving this hard coded to TRUE until we develop something else.
                    $database_settings = <<<PHP
                        
// PROVISION SETTINGS
\$databases['default']['default'] = array(
    'driver' => \$_SERVER['db_type'],
    'database' => \$_SERVER['db_name'],
    'username' => \$_SERVER['db_user'],
    'password' => \$_SERVER['db_passwd'],
    'host' => \$_SERVER['db_host'],
    /* Drupal interprets \$databases['db_port'] as a string, whereas Drush sees
     * it as an integer. To maintain consistency, we cast it to a string. This
     * should probably be fixed in Drush.
     */
    'port' => (string) \$_SERVER['db_port'],
    'charset' => 'utf8mb4',
    'collation' => 'utf8mb4_general_ci',
  );

\$db_url['default'] = \$_SERVER['db_type'] . '://' . \$_SERVER['db_user'] . ':' . \$_SERVER['db_passwd'] . '@' . \$_SERVER['db_host'] . ':' . \$_SERVER['db_port'] . '/' . \$_SERVER['db_name'];

\$settings['hash_salt'] = '$hash_salt';

PHP;
                    $this->fs->appendToFile("$site_path/settings.php", $database_settings);
                }
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

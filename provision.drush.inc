<?php
/**
 * Drush commands
 *
 *
 * This module provides a framework for a Drupal site to manage and install new Drupal sites, using the command line
 * Drush utility. 
 *
 * It allows for pluggable 'provisioning modules' that can extend and modify the tasks that are taken during installation.
 *
 * Each site has the following commands that can be run on it.
 *
 * Implemented : 
 *   install - Install a new Drupal site. The install command uses 3 separate hooks to do it's job, 
 *              namely hook_provision_pre_install(), hook_provision_install() and hook_provision_post_install()
 *   verify  - Recreate all configuration files, to be in synch with changes in the front end. And test that they are correct.
 *   stats   - Return an associated array of site statistics. (implemented in provision_stats module, is thus optional)
 *   import  - Import the details of an already existing site into the provisioning framework.
 *             This command inspects the settings.php and generates the site.php file that the framework uses for configuration.
 *   backup  - Generates a tarball containing the sites directory, the site data configuration and the database dump.
 *             This allows the tarball to act as a 'site package', which can be redeployed on other installations,
 *             or used for an upgrade.
 *   disable - Disable an installed Drupal site. Changes the virtual host config file so that it redirects to provision_disabled_site_redirect_url
 *   enable  - Re-enable a site that has already been disabled. Recreates the virtual host file.
 *   delete -  Generates a back up of the site, and then removes all references to it.
 *   restore - Revert to a previous backup of the site.
 *   cron - Run cron process for a site
 *
 *   upgrade - Accepts a site package (backup) as argument, and redeploys it, running the upgrade processes on it.
 *             Uses hook_provision_pre_upgrade(), hook_provision_upgrade() and hook_provision_post_upgrade() hooks,
 *             and allows clean roll back if any errors occur. Will include stringent checking of module versions,
 *             and allow unit tests to be run.
 * Not implemented yet :
 *   rename  - Change the url of a site. This requires moving of files, and numerous other issues.
 */

/**
 * @defgroup provisiondrush Command line interface for Provision.
 * @{
 */
include_once('provision.inc');
include_once('provision.path.inc');
provision_init();
/** 
 * Implementation of hook_init
 *
 * Provide defines for all the major paths and settings.
 * These are settings that must not be modified during the running of the
 * program, but are configurable.
 */
function provision_init() {
  // Do not allow the program to be run as the root user. ever
  $name = posix_getpwuid(posix_geteuid());
  if ($name['name'] == 'root') {
    return drush_set_error(PROVISION_IS_ROOT, dt('You are running the provision script as the root user. Exiting'));
  }

  // Set up defines for platform
  $docroot = drush_get_option(array("r", "root"), $_SERVER['PWD']);
  
  drush_set_default('provision_domain', $_SERVER['HTTP_HOST']);

  // Paths
  $path = drush_set_default('docroot_path', rtrim(($docroot) ? $docroot : $_SERVER['DOCUMENT_ROOT'], '/'));
  drush_set_default('sites_path', $path . '/sites');

  $parts = explode("/", $path);
  array_pop($parts);
  $parent_path = drush_set_default('parent_path', implode("/" , $parts));


  drush_set_default('backup_path', $parent_path . '/backups');
  drush_set_default('config_path', $parent_path . '/config');

  $config_path = drush_get_option('config_path');
  drush_set_default('vhost_path', $config_path . '/vhost.d');

  // Commands
  drush_set_default('restart_cmd', _provision_default_restart_cmd());

  // System account
  drush_set_default('web_group', _provision_default_web_group());
  drush_set_default('script_user', get_current_user());

  // Redirection urls
  drush_set_default('master_url', $GLOBALS['base_url']);
  $master_url = drush_get_option('master_url');
  drush_set_default('web_disable_url', $master_url .'/provision/disabled');
  drush_set_default('web_maintenence_url', $master_url .'/provision/maintenance');

  // Database
  drush_set_default('master_db', $GLOBALS['db_url']);

  $master_db = drush_get_option('master_db');
  $db = parse_url($master_db);
  drush_set_default('master_db_user', $db['user']);
  drush_set_default('master_db_passwd', $db['pass']);

  drush_set_default('master_db_host', $db['host']);
  drush_set_default('db_host', $db['host']);
 
  drush_set_default('master_db_type', $db['scheme']);
  drush_set_default('db_type', $db['scheme']);

  drush_set_default('publish_path', drush_get_option('docroot_path'));
  drush_set_default('site_url', $url); 
  drush_set_default('profile', 'default');
  drush_set_default('web_ip', '127.0.0.1');
  drush_set_default('web_port', 80); 
  drush_set_default('language', 'en');
  drush_set_default('aliases', array());
}


/**
 * Implementation of hook_drush_command().
 */
function provision_drush_command() {

  $items['provision install'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('install'),
    'arguments' => array('domain.com' => dt('The domain of the site to install.')),
    'description' => dt('Provision a new site using the provided data.'),
    'bootstrap' => -1
  );

  $items['provision import'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('import'),
    'arguments' => array('domain.com' => dt('The domain of the site to import.')),
    'description' => dt('Turn an already running site into a provisioned site.'),
    'bootstrap' => -1
  );

  $items['provision update'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('update'),
    'arguments' => array('domain.com' => dt('The domain of the site to update.')),
    'description' => dt('Run any outstanding updates on the site.'),
    'bootstrap' => -1
  );
  
  $items['provision backup'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('backup'),
    'arguments' => array('domain.com' => dt('The domain of the site to back up.')),
    'optional arguments' => array('backup-file' => dt('The file to save the backup to. This will be a gzipped tarball.')),
    'description' => dt('Generate a back up for the site.'),
    'bootstrap' => -1
  );
  
  $items['provision enable'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('enable'), 
    'arguments' => array('domain.com' => dt('The domain of the site to enable (only if enabled).')),
    'description' => 'Enable a disabled site.',
    'bootstrap' => -1
  );
  $items['provision disable'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('disable'), 
    'arguments' => array('domain.com' => dt('The domain of the site to disable (only if disabled).')),    
    'description' => 'Disable a site.',
    'bootstrap' => -1
  );

  $items['provision verify'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('verify'),
    'arguments' => array('domain.com' => dt('The domain of the site to verify).')),    
    'description' => 'Verify that the provisioning framework is correctly installed.',
    'bootstrap' => -1
  );
 
  $items['provision restore'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('restore'),
    'description' => 'Restore the site to a previous backup. This will also generate a backup of the site as it was.',
    'arguments' => array('domain.com' => dt('The domain of the site to be restored'),
      'site_backup.tar.gz' => dt('The backup to restore the site to.')),
    'bootstrap' => -1
  );

  $items['provision deploy'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('deploy'),
    'description' => 'Deploy an existing backup to a new url.',
    'arguments' => array('domain.com' => dt('The domain to deploy the site package to.'),
      'site_backup.tar.gz' => dt('The backup to deploy.')),
    'bootstrap' => -1
  );

  $items['provision migrate'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('migrate'),
    'description' => 'Migrate a site between platforms.',
    'arguments' => array('domain.com' => dt('The domain to migrate. Any outstanding updates will be run.'),
      '/path/to/platform' => dt('The platform to migrate the site to.')),
    'bootstrap' => -1
  );
 
  $items['provision delete'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('delete'),
    'description' => 'Delete a site.',
    'bootstrap' => -1
  );

  $items['provision cron'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('cron'),
    'description' => 'Run cron process for site.',
    'arguments' => array('domain.com' => dt('The domain of the site to be processed')),
    'bootstrap' => -1
  );
 
  return $items;
}


/**
 * Generate a provision.settings.php file to configure provision
 */
function _provision_generate_config() {
  drush_log(dt("Generating drushrc.php file"));
  provision_path("chmod", drush_get_option('docroot_path') . '/drushrc.php', 0600,
     dt('Changed permissions of drushrc.php to @confirm'),
      dt('Could not change permissions of drushrc.php to @confirm'));

  provision_save_platform_data();
  provision_path("chmod", drush_get_option('docroot_path') . '/drushrc.php', 0400,
    dt('Changed permissions of drushrc.php to @confirm'),
    dt('Could not change permissions of drushrc.php to @confirm'));
  return TRUE;
}



function _provision_default_restart_cmd() {
  $command = '/usr/sbin/apachectl'; # a proper default for most of the world
  foreach (explode(':', $_SERVER['PATH']) as $path) {
    $options[] = "$path/apache2ctl";
    $options[] = "$path/apachectl";
  }
  # try to detect the apache restart command
  $options[] = '/usr/local/sbin/apachectl'; # freebsd
  $options[] = '/usr/sbin/apache2ctl'; # debian + apache2
  $options[] = $command;

  foreach ($options as $test) {
    if (is_executable($test)) {
      $command = $test;
      break;
    }
  }

  return "sudo $command graceful";
}

function _provision_default_web_group() {
  $info = posix_getgrgid(posix_getgid());
  $common_groups = array(
    'httpd',
    'www-data', 
    'apache',
    'nogroup',
    'nobody',
    $info['name']);

  foreach ($common_groups as $group) {
    if (provision_posix_groupname($group)) {
      return $group;
      break;
    }
  }
  return null;
}


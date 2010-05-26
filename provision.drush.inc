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
 *   install - Install a new Drupal site. The install command uses 3 separate hooks to do its job,
 *             namely hook_pre_provision_install(), hook_provision_install() and hook_post_provision_install().
 *   verify  - Recreate all configuration files, to be in synch with changes in the front end. And test that they are correct.
 *   stats   - Return an associated array of site statistics. (implemented in provision_stats module, is thus optional)
 *   import  - Import the details of an already existing site into the provisioning framework.
 *             This command inspects the settings.php and generates the site.php file that the framework uses for configuration.
 *   backup  - Generates a tarball containing the sites directory, the site data configuration and the database dump.
 *             This allows the tarball to act as a 'site package', which can be redeployed on other installations,
 *             or used for an upgrade.
 *   disable - Disable an installed Drupal site. Changes the virtual host config file so that it redirects to provision_disabled_site_redirect_url
 *   enable  - Re-enable a site that has already been disabled. Recreates the virtual host file.
 *   delete  - In a site context: generates a back up of the site, and then removes all references to it.
 *             In a platform context: removes the platform and its vhost config from the server if no sites are currently running on it
 *   restore - Revert to a previous backup of the site.
 *
 *   deploy  - Accepts a site package (backup) as argument, and redeploys it, running the upgrade processes on it.
 *             Uses hook_provision_pre_upgrade(), hook_provision_upgrade() and hook_provision_post_upgrade() hooks,
 *             and allows clean roll back if any errors occur. Will include stringent checking of module versions,
 *             and allow unit tests to be run.
 *   lock    - Lock a platform so that sites cannot be provisioned on it. This does not disable or delete the platform
 *             nor any sites currently provisioned on it.
 *   unlock  - Unlock a platform so that sites can be provisioned on it.
 *
 *   login_reset - Generate a one-time login reset URL.
 *
 * Not implemented yet :
 *   rename  - Change the url of a site. This requires moving of files, and numerous other issues.
 */

// Do not allow the program to be run as the root user. ever
$name = posix_getpwuid(posix_geteuid());
if ($name['name'] == 'root') {
  return drush_set_error('PROVISION_IS_ROOT', dt('You are running the provision script as the root user. Exiting'));
}

/**
 * @defgroup provisiondrush Command line interface for Provision.
 * @{
 */
include_once('provision.inc');

/**
 * implementation of hook_drush_init().
 *
 * This will abort any process running drush if the load is critical.
 *
 * @see provision_load_critical()
 */
function provision_drush_init() {
  $load = sys_getloadavg();
  if (provision_load_critical($load)) {
    drush_set_error('PROVISION_OVERLOAD', dt("load on system too heavy (@load), aborting", array('@load' => join(" ", $load))));
    exit(1);
  }
}

/**
 * Implementation of hook_drush_command().
 */
function provision_drush_command() {
  $items['provision-named-context-save'] = array(
    'arguments' => array(),
    'description' => dt('Save Drush aliases'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['provision-named-context-delete'] = array(
    'arguments' => array(),
    'description' => dt('Delete Drush aliases'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['provision-install'] = array(
    'arguments' => array('domain.com' => dt('The domain of the site to install.')),
    'description' => dt('Provision a new site using the provided data.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-import'] = array(
    'arguments' => array('domain.com' => dt('The domain of the site to import.')),
    'description' => dt('Turn an already running site into a provisioned site.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-backup'] = array(
    'arguments' => array('domain.com' => dt('The domain of the site to back up.')),
    'optional arguments' => array('backup-file' => dt('The file to save the backup to. This will be a gzipped tarball.')),
    'description' => dt('Generate a back up for the site.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );
  
  $items['provision-enable'] = array(
    'arguments' => array('domain.com' => dt('The domain of the site to enable (only if enabled).')),
    'description' => 'Enable a disabled site.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );
  $items['provision-disable'] = array(
    'arguments' => array('domain.com' => dt('The domain of the site to disable (only if disabled).')),    
    'description' => 'Disable a site.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-lock'] = array(
    'description' => 'Lock a platform from having any other sites provisioned on it.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-unlock'] = array(
    'description' => 'Unlock a platform so that sites can be provisioned on it.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-verify'] = array(
    'arguments' => array('domain.com' => dt('The domain of the site to verify).')),    
    'description' => 'Verify that the provisioning framework is correctly installed.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, 
  );
 
  $items['provision-restore'] = array(
    'description' => 'Restore the site to a previous backup. This will also generate a backup of the site as it was.',
    'arguments' => array('domain.com' => dt('The domain of the site to be restored'),
      'site_backup.tar.gz' => dt('The backup to restore the site to.')),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-deploy'] = array(
    'description' => 'Deploy an existing backup to a new url.',
    'arguments' => array('domain.com' => dt('The domain to deploy the site package to.'),
      'site_backup.tar.gz' => dt('The backup to deploy.')),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-migrate'] = array(
    'description' => 'Migrate a site between platforms.',
    'arguments' => array('domain.com' => dt('The domain to migrate. Any outstanding updates will be run.'),
      '/path/to/platform' => dt('The platform to migrate the site to.')),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-clone'] = array(
    'description' => 'Clone a site between platforms.',
    'arguments' => array('domain.com' => dt('The domain to clone. Any outstanding updates will be run.'),
      'new.domain.com' => dt('The new domain name to use.'),
      '/path/to/platform' => dt('The platform to clone the site to.')),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );
 
  $items['provision-delete'] = array(
    'description' => 'Delete a site.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );

  $items['provision-login_reset'] = array(
    'description' => 'Generate a one-time login reset URL.',
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );
  

  $items['hostmaster-migrate'] = array(
    'description' => dt('Migrate an instance of the Hostmaster front end to a new platform'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT,
    'arguments' => array(
      'example.com' => dt('The name of the site to migrate'),
      '/path/to/platform' => dt('The platform to migrate the site to.')
    ),
  );

  $items['hostmaster-make'] = array(
    'description' => dt('Build a platform containing the Hostmaster user interface for provision.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'arguments' => array(
      '/path/to/platform' => dt('The path to create the platform in.')
    ),
  );

  $items['hostmaster-install'] = array(
    'description' => dt('Install and verify the Hostmaster frontend.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    'arguments' => array(
      'example.com' => dt('The name of the site to install'),
      '/path/to/platform' => dt('The platform to install the site on.'),
      'you@example.com' => dt('The email account to send a welcome mail to'),
    ),
  );
  
  $items['hostmaster-pause'] = array(
    'description' => dt('Prepare the site to be migrated to a new platform.'),
    'arguments' => array(
      'example.com' => dt('The url of the site being migrated.')),
  );

  $items['hostmaster-resume'] = array(
    'description' => dt('Complete the migration of the site to a new platform.'),
    'arguments' => array(
      'example.com' => dt('The url of the site being migrated.')),
  );

  return $items;
}

function drush_provision_hostmaster_make($platform) {
  drush_backend_invoke('make', array(dirname(__FILE__) . '/aegir.make', $platform));
}

function drush_http_provision_named_context_save() {
  if (PROVISION_CONTEXT_SERVER) {
    $options = array('provision-host', 'provision-user', 'config_path', 'backup_path');
    foreach (provision_service('all') as $service) {
      $options = array_merge($options, $service->write_alias());
    }
    $config = new provisionConfig_drushrc_alias(drush_get_option('hosting_name'), $options);
    $config->write();
  }
  elseif (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('platform-name', preg_replace('/^platform_/', '', drush_get_option('hosting_name')));
    $config = new provisionConfig_drushrc_alias(drush_get_option('hosting_name'), array('provision-http-server', 'root', 'platform-name'));
    $config->write();
  }
  elseif (PROVISION_CONTEXT_SITE) {
    $config = new provisionConfig_drushrc_alias(drush_get_option('hosting_name'), array('provision-db-server', 'provision-platform', 'uri', 'site_port', 'site_id', 'language', 'client_email'));
    $config->write();
  }
}

function drush_http_provision_named_context_delete() {
  $config = new provisionConfig_drushrc_alias(drush_get_option('hosting_name'));
  $config->unlink();
}

function _provision_default_restart_cmd() {
  $command = '/usr/sbin/apachectl'; // A proper default for most of the world
  foreach (explode(':', $_SERVER['PATH']) as $path) {
    $options[] = "$path/apache2ctl";
    $options[] = "$path/apachectl";
  }
  // Try to detect the apache restart command.
  $options[] = '/usr/local/sbin/apachectl'; // freebsd
  $options[] = '/usr/sbin/apache2ctl'; // debian + apache2
  $options[] = '/usr/apache2/2.2/bin'; // Solaris
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
    'webservd',
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

/**
 * determine the number of CPU on the machine
 *
 * This tries a best guess at the number of CPUs running on the system. This is
 * useful for calculating sane load threshold.
 *
 * On Linux, this parses /proc/cpuinfo and looks for lines like this:
 * 
 * processor	: 0
 * ...
 * processor	: 1
 * processor	: n
 *
 * The number of CPUs on the system is n+1, we just count the number of lines.
 *
 * Other systems remain to be implemented, and would be best implemetend
 * through a PECL (or similar) extension that would use the POSIX sysconf
 * interface, as such:
 *
 * ncpus = sysconf(_SC_NPROCESSORS_ONLN);
 *
 * If no method can be found to figure out the number of CPUs, this will return
 * FALSE.
 *
 * People wishing to extend this to other platforms should look at
 * suggestions at:
 *
 * http://groups.google.com/group/sage-devel/browse_thread/thread/d65209f7ad6057fc
 *
 * @see provision_load_critical()
 * @todo implement for other systems than Linux
 */
function provision_count_cpus() {
  $ncpus = FALSE;
  if (file_exists("/proc/cpuinfo")) {
    # this should work on Linux with a /proc filesystem
    $cpuinfo = file_get_contents("/proc/cpuinfo");
    if ($cpuinfo !== FALSE) {
      if (preg_match_all("/^processor.*:.*[0-9]+$/m", $cpuinfo, $matches)) {
        $ncpus = count(array_pop($matches));
      }
    }
  }
  return $ncpus;
}

/**
 * determine if overall load of the machine is critical
 *
 * We use the "average system load" of the system as a metric, as available
 * through 'uptime' or in PHP sys_getloadavg() since 5.1. The load is usually
 * defined as "the number of processes in the system run queue"
 *
 * It's not a really reliable metric, but it's the best shot we've got without
 * getting into real specific details about I/O, CPU or memory load that are
 * going to be even tougher to evaluate.
 *
 * We base our evaluation on the number of CPUs on the servers. If there are
 * more than 5 processes waiting per CPU, we abort completely. If we ignore the
 * number of available CPUs, we assume a critical limit is a load of 10.
 *
 * @see sys_getloadavg()
 */
function provision_load_critical($load = null, $threshold = null) {
  if (is_null($load)) {
    $load = sys_getloadavg();
  }
  if (is_null($threshold)) {
    if ($ncpus = provision_count_cpus()) {
      $threshold = $ncpus * 5;
    } else {
      // can't determine the number of CPU, we hardcode at load 10
      $threshold = 10;
    }
  }
  return ($load[0] > $threshold);
}


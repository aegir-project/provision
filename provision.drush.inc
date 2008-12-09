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
 * Not implemented yet :
 *   upgrade - Accepts a site package (backup) as argument, and redeploys it, running the upgrade processes on it.
 *             Uses hook_provision_pre_upgrade(), hook_provision_upgrade() and hook_provision_post_upgrade() hooks,
 *             and allows clean roll back if any errors occur. Will include stringent checking of module versions,
 *             and allow unit tests to be run.
 *   rename  - Change the url of a site. This requires moving of files, and numerous other issues.
 */

/**
 * @defgroup provisiondrush Command line interface for Provision.
 * @{
 */

/**
 * Implementation of hook_drush_command().
 */
function provision_drush_command() {
  
  $items['provision install'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('install'),
    'arguments' => array('domain.com' => t('The domain of the site to install.')),
    'description' => t('Provision a new site using the provided data.')
  );

  $items['provision import'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('import'),
    'arguments' => array('domain.com' => t('The domain of the site to import.')),
    'description' => t('Turn an already running site into a provisioned site.')
  );

  $items['provision update'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('update'),
    'arguments' => array('domain.com' => t('The domain of the site to update.')),
    'description' => t('Run any outstanding updates on the site.')
  );
  
  $items['provision backup'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('backup'),
    'arguments' => array('domain.com' => t('The domain of the site to back up.')),
    'optional arguments' => array('backup-file' => t('The file to save the backup to. This will be a gzipped tarball.')),
    'description' => t('Generate a back up for the site.')
  );
  
  $items['provision enable'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('enable'), 
    'arguments' => array('domain.com' => t('The domain of the site to enable (only if enabled).')),
    'description' => 'Enable a disabled site.'
  );
  $items['provision disable'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('disable'), 
    'arguments' => array('domain.com' => t('The domain of the site to disable (only if disabled).')),    
    'description' => 'Disable a site.'
  );

  $items['provision verify'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('verify'),
    'arguments' => array('domain.com' => t('The domain of the site to verify).')),    
    'description' => 'Verify that the provisioning framework is correctly installed.'
  );
 
  $items['provision restore'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('restore'),
    'description' => 'Restore the site to a previous backup. This will also generate a backup of the site as it was.',
    'arguments' => array('domain.com' => t('The domain of the site to be restored'),
                    'site_backup.tar.gz' => t('The backup to restore the site to.'))
  );
 
  $items['provision delete'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('delete'),
    'description' => 'Delete a site.'
  );

  $items['provision cron'] = array(
    'callback' => 'provision_command',
    'callback arguments' => array('cron'),
    'description' => 'Run cron process for site.',
    'arguments' => array('domain.com' => t('The domain of the site to be processed'))
  );
 
  if (!function_exists('hosting_setup')) {
    $items['provision setup'] = array(
      'callback' => '_provision_setup_cmd',
      'description' => 'Initialize this platform to be able to create hosted sites.',
    );
  }

  return $items;
}


/**
 * Initial setup of platform
 * 
 * Creates symlink to drush.php
 * Creates config path
 * Creates drushrc path
 *
 * This function is re-used by the hosting_setup command, as it is a superset of this functionality.
 */
function _provision_setup() {
  $success = TRUE;
  $drush_path = sprintf("%s/%s/drush.php", PROVISION_DOCROOT_PATH, drupal_get_path('module', 'drush'));

  $success &= provision_path('symlink', $drush_path, PROVISION_DOCROOT_PATH . '/drush.php', 
    t('Created symlink for drush.php file'), 
    t('Could not create symlink for drush.php'), 
    PROVISION_FRAMEWORK_ERROR);

  $success &= _provision_generate_config();
  return $success; 
}

/**
 * Drush command wrapper for the setup of the platform
 */
function _provision_setup_cmd() {
  if (_provision_setup()) {
    variable_set('provision_setup', TRUE); 
  }
 
  // @TODO use provision_output for this, but we need pretty print first.
  $logs = provision_get_log();
  foreach ($logs as $log) {
    print "$log[message]\n";
  }

  if (provision_get_error()) {
    print "\nThe command did not complete successfully, please fix the issues and re-run this script.";
  }
}


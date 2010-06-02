<?php

/**
 * Clone command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Execute 'provision deploy' to build the new site using the backup of the old site.
 *
 */

/**
 * Make sure we have a valid site being cloned, and that the file being cloned from exists
 */
function drush_provision_drupal_provision_clone_validate($new_url = null, $platform = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_clone($new_url, $platform = null) {
  drush_invoke('provision-backup');
}

/**
 * Remove the extracted site directory
 */
function drush_provision_drupal_pre_provision_clone_rollback($new_url, $platform = null) {
  $success =  provision_service('file')->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused clone site package')
    ->fail('Could not remove unused clone site package');
}

/**
 * Switch the clone directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_clone($new_url, $platform = null) {
  # note that we reset the aliases so they don't conflict with the original site
  drush_backend_invoke('provision-deploy', array(
    $new_url, 
    drush_get_option('backup_file'), 
    'root' => $platform, 
    'aliases' => drush_get_option('aliases'),
    'redirection' => drush_get_option('redirection'),
    'web_host' => drush_get_option('web_host'), 
    'profile' => drush_get_option('profile'), 
    'db_host' => drush_get_option('db_host')
  ));
}

<?php

/**
 * Rename command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Execute 'provision deploy' to build the new site using the backup of the old site.
 *   3. Delete the old site.
 *
 */

/**
 * Make sure we have a valid site being renamed, and that the site being renamed exists
 */
function drush_provision_drupal_provision_rename_validate($url = null, $new_url = null, $platform = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_rename($url, $new_url, $platform = null) {
  drush_invoke('provision-backup', $url);
}

/**
 * Remove the extracted site directory
 */
function drush_provision_drupal_pre_provision_rename_rollback($url, $new_url, $platform = null) {
  d()->service('file')->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused rename site package')
    ->fail('Could not remove unused site package');
}

/**
 * Switch the rename directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_rename($url, $new_url, $platform = null) {
  # note that we reset the aliases so they don't conflict with the original site
  drush_backend_invoke('provision-deploy', array(
    $new_url, 
    drush_get_option('backup_file'), 
    'root' => $platform, 
    'aliases' => drush_get_option('aliases'),
    'redirection' => drush_get_option('redirection'),
    'web_host' => drush_get_option('web_host'), 
    'web_ip' => drush_get_option('web_ip'), 
    'db_host' => drush_get_option('db_host')
  ));
}

function drush_provision_drupal_post_provision_rename($url) {
  /* @TODO: sort this out.. to do a drush_backend_invoke means 
   * that drush_save_config($context) gets called.. somehow?
   * and it tries to write to the drushrc.php of the old site 
   * that's been deleted
   */
  drush_invoke("provision-delete", $url);
}

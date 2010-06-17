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
function drush_provision_drupal_provision_rename_validate($new_url = null, $platform = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_rename($new_url, $platform = null) {
  /* Set offline mode to true and re-generate the settings.php. This will write a 
   * $conf['site_offline'] = 1; to the settings.php
   */
  drush_log(dt("Putting site under maintenance"));
  drush_set_option('site_offline', 1);
  _provision_drupal_create_settings_file();
  drush_invoke('provision-backup');
}

/**
 * Remove the extracted site directory
 */
function drush_provision_drupal_pre_provision_rename_rollback($new_url, $platform = null) {
  // Set site_offline to false and regenerate the config 
  drush_log(dt("Bringing site out of maintenance"));
  drush_set_option('site_offline', 0);
  $success =  provision_file()->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused migration site package')
    ->fail('Could not remove unused migration site package');
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();
}

/**
 * Switch the rename directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_rename($new_url, $platform = null) {
  drush_set_option('old_platform', d()->platform->name);

  $options = d()->options;
  $options['uri'] = ltrim($new_url, '@');
  $options['platform'] = (isset($platform)) ? $platform : $options['platform'];
  $options['aliases'] = array();
  $options['redirection'] = 0;
  
  drush_backend_invoke_args('provision-save', array($new_url), $options);
  # note that we reset the aliases so they don't conflict with the original site

  provision_backend_invoke($new_url, 'provision-deploy', array(drush_get_option('backup_file')));
  if (!drush_get_error()) {
    provision_backend_invoke($new_url, 'provision-verify');
  }
}

function drush_provision_drupal_post_provision_rename($new_url) {
  drush_set_option('installed', FALSE);
  // we remove the aliases even if redirection is enabled as a precaution
  // if redirection is enabled, keep silent about errors
  _provision_drupal_delete_aliases(d()->aliases);
  _provision_recursive_delete(d()->site_path);
  d()->service('http')->sync(d()->site_path);

  // remove the existing alias 
  $config = new provisionConfig_drushrc_alias(d()->name);
  $config->unlink();

}

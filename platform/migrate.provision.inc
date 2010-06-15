<?php

/**
 * Restore command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Temporarily disable the site by causing apache to redirect to a help page. Restarting apache is required.
 *   3. Extract the backup that is being migrated to to a temporary folder in the sites directory.
 *   4. Create a new database, belonging to the site's user, and switch it around with the current site's database.
 *   5. Import the old database and site.php details.
 *   6. Switch around the sites directory of the current site and the backup being migrated.
 *   7. Regenerate configuration files.
 *   8. TODO: diagnostic to test that everything is ok?
 *   9. Remove the temporary redirect and restart apache so the previous site is available again.
 *   10. Remove the extranuous db and duplicate site directory.
 *
 * If at any time an error occurs, before step 9. It should reverse all the changes it has made,
 * and leave the current site directory and database in the right place, and remove all cruft that
 * was created by this process.
 */

/**
 * Make sure we have a valid site being migrated, and that the file being migrated from exists
 */
function drush_provision_drupal_provision_migrate_validate($platform = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_migrate($platform = null) {
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
 * Restore the vhost conf per the original platform
 */
function drush_provision_drupal_pre_provision_migrate_rollback($platform) {
  // Set site_offline to false and regenerate the config 
  drush_log(dt("Bringing site out of maintenance"));
  drush_set_option('site_offline', 0);
  _provision_drupal_create_settings_file();
  $success =  provision_file()->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused migration site package')
    ->fail('Could not remove unused migration site package');
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();
}

/**
 * Switch the migrate directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_migrate($platform) {
  drush_set_option('old_platform', d()->platform->name);
  drush_backend_invoke_args('provision-save', array(d()->name), array('platform' => $platform, 'root' => null, 'uri' => null));
  provision_backend_invoke(d()->name, 'provision-deploy', array(drush_get_option('backup_file')));
  if (!drush_get_error()) {
    provision_backend_invoke(d()->name, 'provision-verify');
  }
}

/**
 * Something went wrong
 */
function drush_provision_drupal_provision_migrate_rollback($platform) {
  drush_backend_invoke_args('provision-save', array(d()->name), array('platform' => drush_get_option('old_platform'), 'root' => null, 'uri' => null));
}
/**
 * Delete the old site directory and recreate the settings file
 */
function drush_provision_drupal_post_provision_migrate() {
  drush_set_option('installed', FALSE);
  // we remove the aliases even if redirection is enabled as a precaution
  // if redirection is enabled, keep silent about errors
  _provision_drupal_delete_aliases(drush_get_option('aliases', array()), drush_get_option('redirection'));
  _provision_recursive_delete(d()->site_path);
  d()->service('http')->sync(d()->site_path);
}



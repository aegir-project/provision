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
function drush_provision_drupal_provision_migrate_validate($url = null, $platform = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_migrate($url, $platform) {
  drush_set_option('site_offline', 1);
  _provision_drupal_create_settings_file($url);
  drush_invoke('provision backup', $url);
}

/**
 * Remove the extracted site directory
 * Restore the vhost conf per the original platform
 */
function drush_provision_drupal_pre_provision_migrate_rollback($url, $platform) {
  $success =  provision_path("unlink", drush_get_option('backup_file'), TRUE, dt('Removed unused migration site package'), dt('Could not remove unused migration site package'));
  _provision_apache_create_vhost_config($url);
}

/**
 * Switch the migrate directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_migrate($url, $platform) {
  drush_backend_invoke('provision deploy', array($url, drush_get_option('backup_file'), 'root' => $platform, 
    'web_host' => drush_get_option('web_host'), 'web_ip' => drush_get_option('web_ip'), 
    'db_host' => drush_get_option('db_host')));
}


/**
 * Delete the old site directory and recreate the settings file
 */
function drush_provision_drupal_post_provision_migrate($url) {
  drush_set_option('installed', FALSE);
  _provision_drupal_delete_aliases(drush_get_option('aliases', array()));
  _provision_recursive_delete(drush_get_option('sites_path') ."/$url");
}



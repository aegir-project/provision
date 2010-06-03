<?php

/**
 * Restore command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Temporarily disable the site by causing apache to redirect to a help page. Restarting apache is required.
 *   3. Extract the backup that is being restored to to a temporary folder in the sites directory.
 *   4. Create a new database, belonging to the site's user, and switch it around with the current site's database.
 *   5. Import the old database and site.php details.
 *   6. Switch around the sites directory of the current site and the backup being restored.
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
 * Make sure we have a valid site being restored, and that the file being restored from exists
 */
function drush_provision_drupal_provision_restore_validate($restore_file = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

  $exists = provision_service('file')->exists($restore_file)
    ->succeed('Restoring site from @path')
    ->fail('Could not find backup file @path', 'PROVISION_BACKUP_NOT_FOUND')
    ->status();
  if ($exists) {
    drush_set_option('restore_file', $restore_file);
  }
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_restore($restore_file) {
  drush_invoke('provision-backup');

  provision_service('file')->extract(drush_get_option('restore_file'), d()->root . '/sites/' . d()->uri . '.restore') 
    ->succeed('Successfully extracted the contents of @path')
    ->fail('Failed to extract the contents of @path', 'PROVISION_BACKUP_EXTRACTION_FAILED');
}

/**
 * Remove the extracted site directory
 */
function drush_provision_drupal_pre_provision_restore_rollback() {
  _provision_recursive_delete(d()->root . '/sites/' . d()->uri . '.restore');
}

/**
 * Switch the restore directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_restore() {
  $old = d()->root . '/sites/' . d()->uri . '.restore';
  $new = d()->root . '/sites/' . d()->uri;
  provision_service('file')->switch_paths($old, $new)
    ->succeed('Swapping out the @path1 and @path2 directories was successful.')
    ->fail('Swapping the @path1 and @path2 directories has failed.', 'DRUSH_PERM_ERROR');
  // make sure it has the latest site data available
  _provision_drupal_create_settings_file();
  provision_save_site_data();
}

// Luckily this is reversable =)
function drush_provision_drupal_provision_restore_rollback() {
  drush_provision_drupal_provision_restore();
}

/**
 * Delete the old site directory and recreate the settings file
 */
function drush_provision_drupal_post_provision_restore() {
  _provision_recursive_delete(d()->root . '/sites/' . d()->uri . '.restore');
  // This is the actual drupal provisioning requirements. 
  _provision_drupal_create_directories();
  _provision_drupal_create_settings_file();
}



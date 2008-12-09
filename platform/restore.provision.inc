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
function provision_drupal_provision_restore_validate(&$data, $url = null, $restore_file = null) {
  _provision_drupal_url_required();
  _provision_drupal_valid_installed_site();

  $exists = provision_path("exists", $restore_file, TRUE,
      pt("Restoring site from @path"),
      pt("Could not find backup file @path"),
      PROVISION_FRAMEWORK_ERROR);
  if ($exists) {
    $data['restore_file'] = $restore_file;
  }
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function provision_drupal_provision_pre_restore(&$data, $url, $restore_file) {
  provision_invoke('backup', &$data, $url);

  provision_path("extract", $data['restore_file'], PROVISION_SITES_PATH ."/$url.restore", 
    pt('Successfully extracted the contents of @path'),
    pt('Failed to extract the contents of @path'),
    PROVISION_PERM_ERROR, PROVISION_FRAMEWORK_ERROR);
}

/**
 * Remove the extracted site directory
 */
function provision_provision_pre_restore_rollback(&$data, $url) {
  _provision_recursive_delete(PROVISION_SITES_PATH ."/$url.restore");
}

/**
 * Switch the restore directories around now that we have the new db installed
 */
function provision_drupal_provision_restore(&$data, $url) {
  $old = PROVISION_SITES_PATH ."/$url.restore";
  $new = PROVISION_SITES_PATH ."/$url";
  provision_path("switch_paths", $old, $new ,
    pt('Swapping out the @path and @confirm directories was successful.'),
    pt('Swapping the @path and @confirm directories has failed.'),
    PROVISION_PERM_ERROR);
  // make sure it has the latest site data available
  _provision_drupal_create_settings_file($data, $url);
  provision_save_site_data($data, $url);
}

// Luckily this is reversable =)
function provision_drupal_provision_restore_rollback(&$data, $url) {
  provision_drupal_provision_restore(&$data, $url);
}

/**
 * Delete the old site directory and recreate the settings file
 */
function provision_drupal_provision_post_restore(&$data, $url) {
  _provision_recursive_delete(PROVISION_SITES_PATH ."/$url.restore");
  _provision_drupal_create_settings_file($data, $url);
}



<?php

/**
 * Deploy command implementation
 *
 * This command when called will 
 *   1. Extract the backup that is being deployed to the target folder in the sites directory.
 *   2. Import the drushrc.php details.
 *   3. Do some diagnostics to make sure all the needed packages are available.
 *   4. Create a new database, belonging to the site's user, and import the database dump.
 *   5. Regenerate configuration files to show new db settings.
 *   6. Call the drush 'updatedb' command to update the database if neccesary.
 *   7. Update the file paths to the new sites directory.
 *   8. Rebuild the site's package manifest.
 *   9. Save the new drushrc.php with the newly generated settings.
 */

/**
 * Make sure we have a valid site being deployd, and that the file being deployd from exists
 */
function drush_provision_drupal_provision_deploy_validate($url = null, $backup_file = null) {
  _provision_drupal_url_required();

  $exists = provision_path("exists", $backup_file, TRUE,
      dt("Deploying site from @path"),
      dt("Could not find backup file @path"),
      'PROVISION_BACKUP_NOT_FOUND');
  if ($exists) {
    drush_set_option('backup_file', $backup_file);
  }
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_deploy($url, $backup_file) {
  $extracted = provision_path("extract", drush_get_option('backup_file'), drush_get_option('sites_path') ."/$url", 
    dt('Successfully extracted the contents of @path'),
    dt('Failed to extract the contents of @path'),
    'PROVISION_BACKUP_EXTRACTION_FAILED');
  if ($extracted) {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

    $site_packages = drush_get_option('packages', array(), 'site');
    $profiles = array_keys($site_packages['profiles']);
    $profile = $profiles[0];

    $drupal_packages = drush_get_option('packages', array(), 'drupal');

    $merged_modules = array_merge($drupal_packages['base']['modules'], $drupal_packages['profiles'][$profile]['modules']);
    foreach ($site_packages['modules'] as $name => $module) {
      if ($module['status'] == 1) {
        if (!array_key_exists($name, $merged_modules)) {
          drush_log(dt("Could not find a version of the !name module", array('!name' => $name)), 'warning');
        }
        else {
          if ($module['schema_version'] > $merged_modules[$name]['schema_version']) {
            drush_set_error('PROVISION_SCHEMA_UPGRADE_FAILURE', 
              dt("The version of the !name module found on this platform has a lower Schema version than the one the site has installed", 
              array('!name' => $name)));
          }
          else {
            drush_log(dt("Found a valid version of the !name module with schema version !schema_version", 
              array('!name' => $name, '!schema_version' => $merged_modules[$name]['schema_version'])));
          }
        }
      }
    }
  }
}

/**
 * Remove the extracted site directory
 */
function drush_provision_drupal_pre_provision_deploy_rollback($url) {
  if ($site_root = drush_get_context('DRUSH_DRUPAL_SITE_ROOT')) {
    _provision_recursive_delete($site_root);
  }
}

function drush_provision_drupal_provision_deploy($url) {
  _provision_drupal_maintain_aliases($url);
}


function drush_provision_drupal_post_provision_deploy($url) {
  _provision_drupal_create_settings_file($url);
  // call the drush updatedb command.
  drush_backend_invoke("updatedb", array('uri' => "http://$url"));
  // We should be able to fully load Drupal now.
  if (drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL)) {
    drush_include_engine('drupal', 'deploy');
    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
  }
}

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
function drush_provision_drupal_provision_deploy_validate($backup_file = null) {
  $exists = provision_file()->exists($backup_file)
    ->succeed('Deploying site from @path')
    ->fail('Could not find backup file @path', 'PROVISION_BACKUP_NOT_FOUND')
    ->status();
  if ($exists) {
    drush_set_option('backup_file', $backup_file);
  }

  $exists = provision_file()->exists(d()->site_path)
    ->succeed('Replacing the existing site at @path')
    ->status();

  if ($exists) {
    drush_set_option('extract_path', d()->site_path . '.restore');
  }
  else {
    drush_set_option('extract_path', d()->site_path);
  }

  drush_set_option('deploy_replace_site', $exists);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_deploy($backup_file) {
  // the url is likely to have changed in the deployment
  $extracted = provision_file()->extract($backup_file, drush_get_option('extract_path')) 
    ->succeed('Successfully extracted the contents of @path')
    ->fail('Failed to extract the contents of @path to @target', 'PROVISION_BACKUP_EXTRACTION_FAILED')
    ->status();
  if ($extracted) {
    // Make sure the files in the files directory are accessible by the web server.
    provision_file()->chgrp(drush_get_option('extract_path') . '/files', d('@server_master')->web_group, TRUE)
      ->succeed('Changed group ownership of files in <code>@path</code> to @gid')
      ->fail('Could not change group ownership of files in <code>@path</code> to @gid');

    if (drush_get_option('deploy_replace_site', FALSE)) {
      $old = d()->site_path . '.restore';
      $new = d()->site_path;

      $swapped = provision_file()->switch_paths($old, $new)
        ->succeed('Swapping out the @path1 and @path2 directories was successful.')
        ->fail('Swapping the @path1 and @path2 directories has failed.', 'DRUSH_PERM_ERROR')
        ->status();
      if ($swapped) {
        drush_set_option('site_dirs_swapped', TRUE);
      }
      else {
        return false;
      }
    }

    // We have already created a new database. Save the info to the config files.
    provision_prepare_environment();
    provision_save_site_data();

    // Load the newly created information, including re-loading the new db creds.
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
    _provision_drupal_create_settings_file();

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
          if (($merged_modules[$name]['schema_version'] > 0) && ($module['schema_version'] > $merged_modules[$name]['schema_version'])) {
            drush_set_error('PROVISION_SCHEMA_UPGRADE_FAILURE', 
              dt("The version of the !name module found on this platform (!versionB) has a lower Schema version than the one the site has installed (!versionA)", 
              array('!name' => $name, '!versionA' => $module['schema_version'], '!versionB' => $merged_modules[$name]['schema_version'])));
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
function drush_provision_drupal_pre_provision_deploy_rollback() {
  if (drush_get_option('deploy_replace_site', FALSE)) {
    if (drush_get_option('site_dirs_swapped', FALSE)) {

      // swap the site directories back if necessary.
      $old = d()->site_path . '.restore';
      $new = d()->site_path;

       provision_file()->switch_paths($old, $new)
        ->succeed('Swapping out the @path1 and @path2 directories was successful.')
        ->fail('Swapping the @path1 and @path2 directories has failed.', 'DRUSH_PERM_ERROR');

       _provision_drupal_create_settings_file();
       provision_save_site_data();
    }
  }
      
  if (provision_file()->exists(drush_get_option('extract_path'))->status()) {
    _provision_recursive_delete(drush_get_option('extract_path'));
  }
}

function drush_provision_drupal_provision_deploy() {
  _provision_drupal_maintain_aliases();
  _provision_drupal_create_directories();
}

function drush_provision_drupal_post_provision_deploy() {
  // call the drush updatedb command.
  provision_backend_invoke(d()->name, 'updatedb');
  // We should be able to fully load Drupal now.
  if (drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL)) {
    drush_include_engine('drupal', 'deploy');
    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
    _provision_drupal_rebuild_caches();

    // rebuild the node access tables only if necessary
    if (!function_exists("node_access_needs_rebuild") || node_access_needs_rebuild()) {
      node_access_rebuild();
      drush_log(t('Rebuilt node access table'));
    }
  } else {
    drush_log("could not bootstrap drupal after updatedb");
  }


  // remove the restore directory 
  if (!drush_get_error() && drush_get_option('deploy_replace_site', FALSE)) {
    _provision_recursive_delete(drush_get_option('extract_path'));
  }

}

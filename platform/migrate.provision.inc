<?php
/**
 * @file
 * Migrate command implementation
 */

/**
 * Make sure we have a valid site being migrated, and that the file being migrated from exists
 *
 * Implementation of drush_hook_COMMAND_validate().
 */
function drush_provision_drupal_provision_migrate_validate($platform = NULL) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 *
 * Implementation of drush_hook_pre_COMMAND().
 */
function drush_provision_drupal_pre_provision_migrate($platform, $new_name = NULL) {
  // Set offline mode to true and re-generate the settings.php file.
  // This will write a $conf['site_offline'] = 1; to the settings.php file.
  drush_log(dt("Putting site under maintenance"));
  d()->site_enabled = FALSE;
  _provision_drupal_create_settings_file();
  drush_invoke('provision-backup');
  provision_drupal_push_site();

  drush_set_option('old_platform', d()->platform->name);

  // If we are renaming the site, pick this up early.
  if (!is_null($new_name) && ($new_name != d()->name)) {
    drush_set_option('target_name', $new_name);
    drush_set_option('source_name', d()->uri);

    // Detect if the platform stays the same.
    if (d()->platform->name == $platform) {
      drush_log('Replacing the existing site on the same platform');
      // deploying over existing site.
      drush_set_option('deploy_replace_site', TRUE);
    }
  }
  else {
    // The site url stays the same, so the target platform
    // must be different if we run Migrate task, right?
    drush_set_option('target_name', d()->name);
  }
}

/**
 * Remove the extracted site directory
 * Restore the vhost conf per the original platform
 *
 * Implementation of drush_hook_pre_COMMAND_rollback().
 */
function drush_provision_drupal_pre_provision_migrate_rollback($platform, $new_name = NULL) {
  // Set site_offline to false and regenerate the config.
  drush_log(dt("Bringing site out of maintenance"));
  d()->site_enabled = true;
  _provision_drupal_create_settings_file();
  provision_drupal_push_site();

  $success =  provision_file()->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused migration site package')
    ->fail('Could not remove unused migration site package');
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();
}

/**
 * Switch the migrate directories around now that we have the new db installed
 *
 * Implementation of drush_hook_COMMAND().
 */
function drush_provision_drupal_provision_migrate($platform, $new_name = NULL) {

  $target = drush_get_option('target_name');
  $options = array();
  $options['uri'] = d()->uri;

  if (!is_null($new_name) && ($new_name != d()->name)) {
    $options = d()->options;
    $options['uri'] = ltrim($new_name, '@');
    $hash_name = drush_get_option('#name') ? '#name' : 'name';
    $options[$hash_name] = $new_name;
  }
  $options['platform'] = $platform;
  $options['root'] = d($platform)->root;

  if ($profile = drush_get_option('profile', FALSE)) {
    $options['profile'] = $profile;
  }

  if ($db_server = drush_get_option('new_db_server', FALSE)) {
    $options['db_server'] = $db_server;
  }

  drush_set_option('new_site_path', "{$options['root']}/sites/{$options['uri']}");

  drush_invoke_process('@none', 'provision-save', array($target), $options);

  // Build an options array to pass to the deploy task. This allows injection
  // of data into the deploy task. See: https://www.drupal.org/node/2471805
  $deploy_options = array(
    'old_uri' => d()->uri,
    'strict' => 0,
  );
  drush_command_invoke_all_ref('provision_deploy_options_alter', $deploy_options, 'migrate');

  provision_backend_invoke($target, 'provision-deploy', array(drush_get_option('backup_file')), $deploy_options);
  if (!drush_get_error()) {
    drush_log(dt("Bringing site out of maintenance"));
    d()->site_enabled = TRUE;
    drush_set_option('provision_save_config', FALSE);
    provision_backend_invoke($target, 'provision-verify');
  }
}

/**
 * Something went wrong
 *
 * Implementation of drush_hook_COMMAND_rollback().
 */
function drush_provision_drupal_provision_migrate_rollback($platform) {
  drush_invoke_process('@none', 'provision-save', array(d()->name), array('platform' => drush_get_option('old_platform'), 'root' => NULL, 'uri' => NULL));
}

/**
 * Delete the old site directory and recreate the settings file
 *
 * Implementation of drush_hook_post_COMMAND().
 */
function drush_provision_drupal_post_provision_migrate($platform, $new_name = NULL) {
  drush_set_option('installed', FALSE);

  if (!drush_get_option('deploy_replace_site', FALSE)) {
    // we remove the aliases even if redirection is enabled as a precaution
    // if redirection is enabled, keep silent about errors
    _provision_drupal_delete_aliases();
    _provision_recursive_delete(d()->site_path);
    d()->service('http')->sync(d()->site_path);
  }



  if (!is_null($new_name) && ($new_name != d()->name)) {
    // remove the existing alias
    $config = new Provision_Config_Drushrc_Alias(d()->name);
    $config->unlink();

    // Remove the symlink in the clients directory.
    _provision_client_delete_symlink();
  }

  // Load the config file of the newly migrated site and return it to hosting.
  provision_reload_config('site', drush_get_option('new_site_path', d()->site_path) . '/drushrc.php');
}

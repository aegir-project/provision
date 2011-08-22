<?php

/**
 * Migrate command implementation
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
function drush_provision_drupal_pre_provision_migrate($platform, $new_uri = null) {
  /* Set offline mode to true and re-generate the settings.php. This will write a 
   * $conf['site_offline'] = 1; to the settings.php
   */
  drush_log(dt("Putting site under maintenance"));
  d()->site_enabled = false;
  _provision_drupal_create_settings_file();
  drush_invoke('provision-backup');

  drush_set_option('old_platform', d()->platform->name);

  // If we are renaming the site, pick this up early.
  if (!is_null($new_uri) && ($new_uri != d()->uri)) {
    drush_set_option('target_name', $new_uri);
  }
  else {
    // the site url stays the same
    drush_set_option('target_name', d()->name);

    // The platform stays the same
    if (d()->platform->name == $platform) {
      drush_log('Replacing the existing site on the same platform');
      // deploying over existing site.
      drush_set_option('deploy_replace_site', TRUE);
    }
  }
}

/**
 * Remove the extracted site directory
 * Restore the vhost conf per the original platform
 */
function drush_provision_drupal_pre_provision_migrate_rollback($platform, $new_uri = null) {
  // Set site_offline to false and regenerate the config 
  drush_log(dt("Bringing site out of maintenance"));
  d()->site_enabled = true;
  _provision_drupal_create_settings_file();
  $success =  provision_file()->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused migration site package')
    ->fail('Could not remove unused migration site package');
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();

}

/**
 * Switch the migrate directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_migrate($platform, $new_uri = null) {

  $target = drush_get_option('target_name');

  $options = array();

  $options['uri'] = d()->uri;

  if (!is_null($new_uri) && ($new_uri != d()->uri)) {
    $options = d()->options;
    $options['uri'] = ltrim($new_uri, '@');
    $hash_name = drush_get_option('#name') ? '#name' : 'name';
    $options[$hash_name] = $new_uri;
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

  drush_backend_invoke_args('provision-save', array($target), $options);
  provision_backend_invoke($target, 'provision-deploy', array(drush_get_option('backup_file')), array('old_uri' => d()->uri));
  if (!drush_get_error()) {
    drush_log(dt("Bringing site out of maintenance"));
    d()->site_enabled = true;
    drush_set_option('provision_save_config', false);
    provision_backend_invoke($target, 'provision-verify');
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
function drush_provision_drupal_post_provision_migrate($platform, $new_uri = null) {
  drush_set_option('installed', FALSE);

  if (!drush_get_option('deploy_replace_site', FALSE)) {
    // we remove the aliases even if redirection is enabled as a precaution
    // if redirection is enabled, keep silent about errors
    _provision_drupal_delete_aliases();
    _provision_recursive_delete(d()->site_path);
    d()->service('http')->sync(d()->site_path);
  }



  if (!is_null($new_uri) && ($new_uri != d()->uri)) {
    // remove the existing alias 
    $config = new provisionConfig_drushrc_alias(d()->name);
    $config->unlink();
  }

  // Load the config file of the newly migrated site and return it to hosting.
  provision_reload_config('site', drush_get_option('new_site_path', d()->site_path) . '/drushrc.php');
}


<?php

/**
 * Clone command implementation
 *
 * This command when called will 
 *   1. Make a backup of the current site, before modifications are made.
 *   2. Execute 'provision deploy' to build the new site using the backup of the old site.
 *
 */

/**
 * Make sure we have a valid site being cloned, and that the file being cloned from exists
 */
function drush_provision_drupal_provision_clone_validate($new_name = null, $platform = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

/**
 * Make a backup before making any changes, and add extract the file we are restoring from
 */
function drush_provision_drupal_pre_provision_clone($new_name, $platform = null) {
  drush_invoke('provision-backup');
}

/**
 * Remove the extracted site directory
 */
function drush_provision_drupal_pre_provision_clone_rollback($new_name, $platform = null) {
  $success =  provision_file()->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused clone site package')
    ->fail('Could not remove unused clone site package');
}

/**
 * Switch the clone directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_clone($new_name, $platform = null) {
  drush_set_option('old_platform', d()->platform->name);

  // If the site is cloned between platforms and not just in the same platform,
  // we should update the info collected about source and target platform first.
  if (!is_null(d($platform)->name) && (d($platform)->name != d()->platform->name)) {
    provision_backend_invoke('@hostmaster', 'hosting-task', array(d()->platform->name, 'verify'), array('force' => TRUE));
    sleep(5); // A small trick to avoid high load and race conditions.
    provision_backend_invoke('@hostmaster', 'hosting-task', array(d($platform)->name, 'verify'), array('force' => TRUE));
    sleep(5); // A small trick to avoid high load and race conditions.
  }
  // We should update also the info collected about the site before running clone.
  $local_uri_verify = '@' . d()->uri;
  provision_backend_invoke('@hostmaster', 'hosting-task', array($local_uri_verify, 'verify'), array('force' => TRUE));
  sleep(5); // A small trick to avoid high load and race conditions.

  $options = d()->options;
  $options['uri'] = ltrim($new_name, '@');
  $hash_name = drush_get_option('#name') ? '#name' : 'name';
  $options[$hash_name] = $new_name;
  $options['platform'] = (isset($platform)) ? $platform : $options['platform'];
  $options['root'] = d($options['platform'])->root;
  $options['aliases'] = array();
  $options['redirection'] = 0;

  // XXX: right now we just drop SSL configuration when cloning, because
  // we do not check if the site name change is compatible with the
  // certificate. This can be removed when we a) check for wildcard
  // certs and b) allow the user to change the cert from the UI.
  $options['ssl_enabled'] = 0;
  unset($options['ssl_key']);

  // Do not copy CDN aliases to the cloned site --CDN vhost
  $options['cdn'] = 0;
  unset($options['cdn']);

  if ($profile = drush_get_option('profile', FALSE)) {
    $options['profile'] = $profile;
  }

  if ($db_server = drush_get_option('new_db_server', FALSE)) {
    $options['db_server'] = $db_server;
  }

  drush_invoke_process('@none', 'provision-save', array($new_name), $options);
  # note that we reset the aliases so they don't conflict with the original site

  provision_backend_invoke($new_name, 'provision-deploy', array(drush_get_option('backup_file')), array('old_uri' => d()->uri));

  if (!drush_get_error()) {
    // Verify the newly cloned site.
    provision_backend_invoke($new_name, 'provision-verify');
    sleep(5); // A small trick to avoid high load and race conditions.
    // Verify again also original site via frontend to avoid issue #1004526
    provision_backend_invoke('@hostmaster', 'hosting-task', array($local_uri_verify, 'verify'), array('force' => TRUE));
  }

  provision_reload_config('site', d()->site_path . '/drushrc.php');

}

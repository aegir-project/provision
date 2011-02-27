<?php

/**
 * Make sure we have a valid site being restored, and that the file being restored from exists
 */
function drush_provision_drupal_provision_restore_validate($restore_file = null) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

  $exists = provision_file()->exists($restore_file)
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
  drush_log(dt("Putting site under maintenance"));
  d()->site_enabled = false;
  _provision_drupal_create_settings_file();
  drush_invoke('provision-backup');
}

function drush_provision_drupal_pre_provision_restore_rollback($restore_file) {
  $success =  provision_file()->unlink(drush_get_option('backup_file'))
    ->succeed('Removed unused migration site package')
    ->fail('Could not remove unused migration site package');
}

/**
 * Switch the restore directories around now that we have the new db installed
 */
function drush_provision_drupal_provision_restore($restore_file) {

  provision_backend_invoke(d()->name, 'provision-deploy', array(drush_get_option('restore_file')), array('old_uri' => d()->uri));
  if (!drush_get_error()) {
    provision_backend_invoke(d()->name, 'provision-verify');
    drush_set_option('installed', false);
  }
}


<?php

/**
 * Provision backup command
 *
 * Back up an existing site
 */

/**
 * Make sure the site is installed and enabled, and that we have a valid target to back up to.
 */
function drush_provision_drupal_provision_backup_validate($backup_file = NULL) {
  if (!@drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION)) {
    if (drush_get_option('force', false)) {
      drush_log("clearing error");
      drush_set_context('DRUSH_ERROR_CODE', DRUSH_SUCCESS);
    }
  }
  if (!drush_get_option('installed') && !drush_get_option('force', false)) {
     drush_set_error('PROVISION_DRUPAL_SITE_NOT_FOUND');
   }

  // This is the actual drupal provisioning requirements. 
  if (!is_dir(d()->platform->server->backup_path)) {
    drush_set_error('PROVISION_BACKUP_PATH_NOT_FOUND');
  }

  if ($backup_file) {
    if (provision_file()->exists($backup_file)->status()) {
      drush_set_error('PROVISION_BACKUP_ALREADY_EXISTS', dt('Back up file @path already exists.', array('@path' => $backup_file)));
    }
    else {
      drush_log(dt('Backing site up to @path.', array('@path' => $backup_file)));
      drush_set_option('backup_file', $backup_file);
    }
  }

  if (!$backup_file) {
    $suggested = d()->platform->server->backup_path . '/' . d()->uri . '-' . date("Ymd.His", mktime()) . '.tar.gz';

    // Use format of mysite.com-2008-01-02, if already existing, add number.
    while (is_file($suggested)) {
      $count++;
      $suggested = d()->platform->server->backup_path . '/' . d()->uri . '-' .  date('Ymd.His', mktime()) . '_' . $count . '.tar.gz';
    }
    drush_set_option('backup_file', $suggested);
  }

}

/**
 * Implentation of hook_provision_backup()
 */
function drush_provision_drupal_provision_backup() {
  $backup_file = drush_get_option('backup_file');
  // Adds the site directory into the backup file
  drush_log(dt("Adding sites directory to !backup_file", array('!backup_file' => $backup_file)), 'backup');


  // synch all filesystem changes back from the remote server.
  d()->service('http')->fetch(d()->site_path . '/files/');
  d()->service('http')->fetch(d()->site_path . '/private/');
  d()->service('http')->fetch(d()->site_path . '/modules/');
  d()->service('http')->fetch(d()->site_path . '/themes/');
  d()->service('http')->fetch(d()->site_path . '/libraries/');
  d()->service('http')->fetch(d()->site_path . '/local.settings.php');

  // Check if we are currently cloaking credentials
  $cloaked = d()->service('http')->cloaked_db_creds();
  $cloaked = drush_get_option('provision_db_cloaking', $cloaked);

  if ($cloaked) {
    drush_set_option('cloaking_off_temp', TRUE);
    // Disable the cloaking of credentials temporarily
    drush_log(dt("Temporarily uncloaking database credentials for backup"));
    drush_set_option('provision_db_cloaking', FALSE);

    // Write the uncloaked credentials to the settings.php
    _provision_drupal_create_settings_file();
  }

  $olddir = getcwd();
  // we need to do this because some retarded implementations of tar (e.g. SunOS) don't support -C

  if (!chdir(d()->site_path)) {
    return drush_set_error('PROVISION_BACKUP_PATH_NOT_FOUND', dt('cannot change directory to %dir', array('%dir' => d()->site_path)));
  }
  if (substr($backup_file, -2) == 'gz') {
    // same as above: some do not support -z
    $command = "tar cpf - . | gzip -c > %s";
  } else {
    $command = "tar cpf %s .";
  }
  $result = drush_shell_exec($command,  $backup_file);

  // Get the size of the backup
  $size = filesize($backup_file);
  drush_set_option('backup_file_size', $size);

  chdir($olddir);

  if (drush_get_option('cloaking_off_temp', FALSE)) {
    drush_log(dt("Re-cloaking database credentials after backup"));
    drush_set_option('provision_db_cloaking', TRUE);
    _provision_drupal_create_settings_file();
  }

  if (!$result && !drush_get_option('force', false)) {
    drush_set_error('PROVISION_BACKUP_FAILED', dt("Could not back up sites directory for drupal"));
  }
}

function drush_provision_drupal_post_provision_backup() {
  drush_log(dt('Backed up site up to @path.', array('@path' => drush_get_option('backup_file'))), 'success');
  if (d()->client_name) {
    $backup_dir = d()->server->clients_path . '/' . d()->client_name . '/backups';
    provision_file()->create_dir($backup_dir, dt('Client backup directory for @client', array('@client' => d()->client_name)), 0750);
    provision_file()->symlink(drush_get_option('backup_file'), $backup_dir . '/' . basename(drush_get_option('backup_file')))
      ->succeed('Created symlink @path to @target')
      ->fail('Could not create symlink @path to @target: @reason');
  }
}

/**
 * Remove the backup file if something went wrong
 */
function drush_provision_drupal_provision_backup_rollback() {
  $backup_file = drush_get_option('backup_file');
  if (file_exists($backup_file)) {
    provision_file()->unlink($backup_file)
      ->succeed('Removed stale backup file @path')
      ->fail('Failed deleting backup file @path');
  }
}

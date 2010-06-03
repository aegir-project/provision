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
  if (!is_dir(drush_get_option('backup_path'))) {
    drush_set_error('PROVISION_BACKUP_PATH_NOT_FOUND');
  }

  if ($backup_file) {
    if (provision_service('file')->exists($backup_file)->status()) {
      drush_set_error('PROVISION_BACKUP_ALREADY_EXISTS', dt('Back up file @path already exists.', array('@path' => $backup_file)));
    }
    else {
      drush_log(dt('Backing site up to @path.', array('@path' => $backup_file)), 'message');
      drush_set_option('backup_file', $backup_file);
    }
  }

  if (!$backup_file) {
    $suggested = drush_get_option('backup_path') . '/' . drush_get_option('uri') . '-' . date("Ymd.His", mktime()) . '.tar.gz';

    // Use format of mysite.com-2008-01-02, if already existing, add number.
    while (is_file($suggested)) {
      $count++;
      $suggested = drush_get_option('backup_path') . '/' . drush_get_option('uri') . '-' . date('Ymd.His', mktime()) . '_' . $count . '.tar.gz';
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
  $olddir = getcwd();
  // we need to do this because some retarded implementations of tar (e.g. SunOS) don't support -C
  chdir(d()->root . '/sites/' . d()->uri);
  if (substr($backup_file, -2) == 'gz') {
    // same as above: some do not support -z
    $command = "tar cpf - . | gzip -c > %s";
  } else {
    $command = "tar cpf %s .";
  }
  $result = provision_shell_exec($command,  $backup_file);
  chdir($olddir);

  if (!$result && !drush_get_option('force', false)) {
    drush_set_error('PROVISION_BACKUP_FAILED', dt("Could not back up sites directory for drupal"));
  }
}

/**
 * Remove the backup file if something went wrong
 */
function drush_provision_drupal_provision_backup_rollback() {
  $backup_file = drush_get_option('backup_file');
  if (file_exists($backup_file)) {
    provision_service('file')->unlink($backup_file)
      ->succeed('Removed stale backup file @path')
      ->fail('Failed deleting backup file @path');
  }
}

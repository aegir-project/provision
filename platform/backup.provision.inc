<?php

/**
 * Provision backup command
 *
 * Back up an existing site
 */

/**
 * Make sure the site is installed and enabled, and that we have a valid target to back up to.
 */
function drush_provision_drupal_provision_backup_validate($url = NULL, $backup_file = NULL) {
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
    if (provision_path("exists", $backup_file, FALSE,
            dt("Backing site up to @path."),
            dt("Back up file @path already exists."),
        'PROVISION_BACKUP_ALREADY_EXISTS')) {
      drush_set_option('backup_file', $backup_file);
    }
  }

  if (!$backup_file) {
    $suggested = drush_get_option('backup_path') ."/$url-". date("Ymd.His", mktime()) .".tar.gz";

    // Use format of mysite.com-2008-01-02, if already existing, add number.
    while (is_file($suggested)) {
      $count++;
      $suggested = drush_get_option('backup_path') ."/$url-". date("Ymd.His", mktime()) ."_$count.tar.gz";
    }
    drush_set_option('backup_file', $suggested);
  }

}

/**
 * Implentation of hook_provision_backup()
 */
function drush_provision_drupal_provision_backup($url) {
  $backup_file = drush_get_option('backup_file');
  // Adds the site directory into the backup file
  drush_log(dt("Adding sites directory to !backup_file", array('!backup_file' => $backup_file)), 'backup');
  if (substr($backup_file, -2) == 'gz') {
    $command = "tar -C %s -p -c -z -f %s .";
  } else {
    $command = "tar -C %s -p -c -f %s .";
  }
  $result = provision_shell_exec($command, drush_get_option('sites_path') . "/$url",  $backup_file);

  if (!$result && !drush_get_option('force', false)) {
    drush_set_error('PROVISION_BACKUP_FAILED', dt("Could not back up sites directory for drupal"));
  }
}

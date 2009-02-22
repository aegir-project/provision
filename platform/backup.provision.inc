<?php

/**
 * Provision backup command
 *
 * Back up an existing site
 */

/**
 * Make sure the site is installed and enabled, and that we have a valid target to back up to.
 */
function provision_drupal_provision_backup_validate(&$data, $url = NULL, $backup_file = NULL) {
  _provision_drupal_url_required();
  _provision_drupal_valid_installed_site();

  // This is the actual drupal provisioning requirements. 
  if (!is_dir(PROVISION_BACKUP_PATH)) {
    drush_set_error(PROVISION_BACKUP_PATH_NOT_FOUND);
  }
  
  if ($backup_file) {
    if ( provision_path("exists", $backup_file, FALSE,
            dt("Backing site up to @path."),
            dt("Back up file @path already exists."),
        PROVISION_FRAMEWORK_ERROR)) {
      $data['backup_file'] = $backup_file;
    }
  }
  if (!$backup_file) {
    $suggested = PROVISION_BACKUP_PATH ."/$url-". date("Y-m-d", mktime()) .".tar";

    // Use format of mysite.com-2008-01-02, if already existing, add number.
    while (is_file($suggested .'.gz')) {
      $count++;
      $suggested = PROVISION_BACKUP_PATH ."/$url-". date("Y-m-d", mktime()) ."_$count.tar";
    }

    $data['backup_file'] = (!empty($file)) ? ereg_replace('.gz$', '', $file) : $suggested;
  }
}

/**
 * Implentation of hook_provision_backup()
 */
function provision_drupal_provision_backup(&$data, $url, $backup_file) {
  // Adds the site directory into the backup file
  drush_log("Adding sites directory to $data[backup_file].gz", 'backup');
  $result = provision_shell_exec("cd %s; tar -rf %s * ", "sites/$url",  $data['backup_file']);

  if (!$result) {
    drush_set_error(PROVISION_BACKUP_FAILED, dt("Could not back up sites directory for drupal"));
  }
}


/**
 * Generate a backup tarbal for a site.
 */
function provision_drupal_provision_post_backup(&$data, $url, $backup_file = NULL) {
  provision_shell_exec("gzip %s", $data['backup_file']);
  $data['backup_file'] = $data['backup_file'] .'.gz';
}

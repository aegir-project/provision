<?php

/**
 * Provision backup command
 *
 * Back up an existing site
 */

/**
 * Make sure the site is installed and enabled, and that we have a valid target to back up to.
 */
function provision_drupal_provision_backup_validate($url = NULL, $backup_file = NULL) {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
  if (!drush_get_option('installed')) {
     drush_set_error(PROVISION_DRUPAL_SITE_FOUND);
   }

  // This is the actual drupal provisioning requirements. 
  if (!is_dir(drush_get_option('backup_path'))) {
    drush_set_error(PROVISION_BACKUP_PATH_NOT_FOUND);
  }

  if ($backup_file) {
    if ( provision_path("exists", $backup_file, FALSE,
            dt("Backing site up to @path."),
            dt("Back up file @path already exists."),
        PROVISION_BACKUP_ALREADY_EXISTS)) {
      drush_get_option('backup_file', $backup_file);
    }
  }

  if (!$backup_file) {
    $suggested = drush_get_option('backup_path') ."/$url-". date("Y-m-d", mktime()) .".tar";

    // Use format of mysite.com-2008-01-02, if already existing, add number.
    while (is_file($suggested .'.gz')) {
      $count++;
      $suggested = drush_get_option('backup_path') ."/$url-". date("Y-m-d", mktime()) ."_$count.tar";
    }

    drush_set_option('backup_file', str_replace(".gz" , '', $suggested));
  }
}

/**
 * Implentation of hook_provision_backup()
 */
function provision_drupal_provision_backup($url) {
  $backup_file = drush_get_option('backup_file');
  // Adds the site directory into the backup file
  drush_log(dt("Adding sites directory to !backup_file", array('!backup_file' => $backup_file)), 'backup');
  $result = provision_shell_exec("cd %s; tar -rf %s * ", drush_get_option('sites_path') . "/$url",  $backup_file);

  if (!$result) {
    drush_set_error(PROVISION_BACKUP_FAILED, dt("Could not back up sites directory for drupal"));
  }
}


/**
 * Generate a backup tarbal for a site.
 */
function provision_drupal_provision_post_backup($url, $backup_file = NULL) {
  $backup_file = drush_get_option('backup_file');

  provision_shell_exec("gzip %s", $backup_file);
  drush_set_option('backup_file', $backup_file . '.gz');
}

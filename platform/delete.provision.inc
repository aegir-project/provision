<?php


function provision_drupal_provision_delete_validate($url = NULL, $backup_file = NULL) {
  _provision_drupal_valid_site();
}


/**
 * Before starting to delete the site, make a backup
 */
function provision_drupal_provision_pre_delete($url, $backup_file) {
  // @TODO : fix the inline calling of provision_invoke
  provision_invoke('backup', $url, $backup_file);
}

/**
 * Remove any directories for the site in sites
 * This can't be rolled back. so won't even try.
 */
function provision_drupal_provision_delete($url) {
  $old_aliases = drush_get_option('aliases', array(), 'site');

  if (sizeof($old_aliases) == sizeof(drush_get_option('aliases'))) {
    _provision_drupal_delete_aliases($old_aliases);
  }

  _provision_recursive_delete(drush_get_option('sites_path') . "/$url");
  drush_set_option('installed', FALSE, 'site');
}



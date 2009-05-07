<?php


function drush_provision_drupal_provision_delete_validate($url = NULL, $backup_file = NULL) {
  _provision_drupal_valid_site();
}


/**
 * Before starting to delete the site, make a backup
 */
function drush_provision_drupal_pre_provision_delete($url, $backup_file) {
  drush_invoke('provision backup', $url, $backup_file);
}

/**
 * Remove any directories for the site in sites
 * This can't be rolled back. so won't even try.
 */
function drush_provision_drupal_provision_delete($url) {
  _provision_recursive_delete(drush_get_option('sites_path') . "/$url");
  _provision_drupal_delete_aliases(drush_get_option('aliases', array()));
  drush_set_option('installed', FALSE, 'site');
}



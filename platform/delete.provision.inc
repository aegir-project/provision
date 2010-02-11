<?php


function drush_provision_drupal_provision_delete_validate($url = NULL, $backup_file = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_drupal_valid_site();
  }
}

/**
 * Before starting to delete the site, make a backup
 */
function drush_provision_drupal_pre_provision_delete($url = NULL, $backup_file = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    drush_set_option('force', true, 'process');
    drush_invoke("provision-backup", $url, $backup_file);
    drush_unset_option('force', 'process');
  }
}

/**
 * If we're deleting a site, remove any directories for the site in sites folder
 * If we're deleting a platform, remove the whole platform
 * This can't be rolled back. so won't even try.
 */
function drush_provision_drupal_provision_delete($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_recursive_delete(drush_get_option('sites_path') . "/$url");
    // we remove the aliases even if redirection is enabled as a precaution
    // if redirection is enabled, keep silent about errors
    _provision_drupal_delete_aliases(drush_get_option('aliases', array()), drush_get_option('redirection'));
    drush_set_option('installed', FALSE, 'site');
  }
  if (PROVISION_CONTEXT_PLATFORM) {
    $sites = provision_drupal_find_sites();
    if($sites) {
      drush_set_error(dt('Existing sites were found on this platform. These sites will need to be deleted before this platform can be deleted.'));
    }
    else {
      $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
      _provision_recursive_delete($drupal_root);
    }
  }
}



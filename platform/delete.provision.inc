<?php


function provision_drupal_provision_delete_validate(&$data, $url = NULL, $backup_file = NULL) {
  _provision_drupal_valid_site();
}


/**
 * Before starting to delete the site, make a backup
 */
function provision_drupal_provision_pre_delete(&$data, $url, $backup_file) {
  // @TODO : fix the inline calling of provision_invoke
  provision_invoke('backup', $data, $url, $backup_file);
}

/**
 * Remove any directories for the site in sites
 * This can't be rolled back. so won't even try.
 */
function provision_drupal_provision_delete(&$data, $url) {
  if ($old_data = provision_load_site_data($url)) {
    if (sizeof($old_aliases = $old_data['aliases'])) {
      _provision_drupal_delete_aliases($old_aliases);
    }
  }

  _provision_recursive_delete("sites/$url");
  $data['installed'] = FALSE;
}



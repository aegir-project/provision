<?php

/**
 * Before starting to delete the site, make a backup
 */
function drush_provision_drupal_pre_provision_delete($backup_file = NULL) {
  if (d()->type === 'site') {
    drush_set_option('force', true, 'process');
    drush_invoke("provision-backup", $backup_file);
    drush_unset_option('force', 'process');
  }
}

/**
 * If we're deleting a site, remove any directories for the site in sites folder
 * If we're deleting a platform, remove the whole platform
 * This can't be rolled back. so won't even try.
 */
function drush_provision_drupal_provision_delete() {
  if (d()->type === 'site') {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT);

    // '@none' is only supported in Drush 4.6+. See: https://drupal.org/node/2044251
    //drush_invoke_process('@none', 'provision-save', array(d()->name), array('delete' => TRUE));
    drush_invoke_process('provision-save', array(d()->name), array('delete' => TRUE));

    _provision_recursive_delete(d()->site_path);
    // we remove the aliases even if redirection is enabled as a precaution
    // if redirection is enabled, keep silent about errors
    _provision_drupal_delete_aliases();

    // Remove the symlink in the clients directory.
    _provision_client_delete_symlink();

    provision_drupal_sync_site();
  }
  elseif (d()->type === 'platform') {
    if (!drush_get_option('force', FALSE) && drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT) && provision_drupal_find_sites()) {
      drush_set_error(dt('Existing sites were found on this platform. These sites will need to be deleted before this platform can be deleted.'));
    }
    else {
      _provision_recursive_delete(d()->root);
      d()->service('http')->sync(d()->root);
    }
  }
}

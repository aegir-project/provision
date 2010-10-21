<?php

// $Id$

/**
 * Provision export command
 *
 * Create a tarball of a site that can be used on non-aegir hosts
 */

/**
 * Make a backup of the site
 */
function drush_provision_drupal_provision_export($backup_file = null) {
  if (d()->type == 'site') {
    drush_set_option('force', true, 'process');

    // Disable the cloaking of credentials temporarily
    drush_set_option('provision_db_cloaking', FALSE);
    // Write the uncloaked credentials to the settings.php
    _provision_drupal_create_settings_file();

    // Create the tarball   
    drush_invoke('provision-backup', $backup_file);

    // Put the cloaked credentials back
    drush_set_option('provision_db_cloaking', TRUE);
    _provision_drupal_create_settings_file();
    drush_unset_option('force', 'process');
  }
}


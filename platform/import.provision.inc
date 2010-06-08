<?php

/**
 * Provision import command
 *
 * Import existing drupal sites
 */

/**
 * Check that the site directory at least exists.
 */
function drush_provision_drupal_provision_import_validate() {
  _provision_drupal_url_required();
  _provision_drupal_valid_site();
}

/**
 * Import the information about the existing site, and return it in the $data context array
 */
function drush_provision_drupal_provision_import() {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  drush_include_engine('drupal', 'import', drush_drupal_major_version());
  drush_set_option('installed', TRUE, 'site');
}


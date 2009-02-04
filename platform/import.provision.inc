<?php

/**
 * Provision import command
 *
 * Import existing drupal sites
 */

/**
 * Check that the site directory at least exists.
 */
function provision_drupal_provision_import_validate() {
  _provision_drupal_url_required();
  _provision_drupal_valid_site();
}

/**
 * Import the information about the existing site, and return it in the $data context array
 */
function provision_drupal_provision_import(&$data, $url = NULL) {
  provision_internal_init($url);
  provision_platform_include(dirname(__FILE__), 'import');
}


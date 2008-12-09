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
  $info = _provision_drupal_import_site($data, $url);
  if ($info['installed']) {
    provision_log("notice", "Returning information for $url");
  }
}


<?php

/**
 * Provision enable command
 *
 * Enable a site that has been disabled
 */
function provision_drupal_provision_enable_validate() {
  _provision_drupal_url_required();
  _provision_drupal_valid_installed_site();
}

function provision_drupal_post_enable() {
  drush_set_option('enabled', TRUE, 'site');
}



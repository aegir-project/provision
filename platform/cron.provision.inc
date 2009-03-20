<?php

/**
 * Provision cron command
 */

function provision_drupal_provision_cron_verify() {
  _provision_drupal_url_required();
  _provision_drupal_valid_installed_site();
}

/**
 * Drush command to run cron
 */
function _provision_cron($url) {
  _provision_drupal_rebuild_caches($url);
}


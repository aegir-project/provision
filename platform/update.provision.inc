<?php

function provision_drupal_provision_update_validate() {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

function provision_drupal_provision_update($url) {
  _provision_drupal_create_settings_file($url);
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
  provision_platform_include(dirname(__FILE__), 'update');
  provision_platform_include(dirname(__FILE__), 'clear');
}

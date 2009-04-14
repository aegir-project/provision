<?php

function provision_drupal_provision_update_validate() {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
}

function provision_drupal_provision_update($url) {
  _provision_drupal_create_settings_file($url);
  drush_backend_invoke("update", array('uri' => "http://$url"));
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
#  drush_include_engine('drupal', 'update');
#  drush_include_engine('drupal', 'clear');
}

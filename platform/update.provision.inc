<?php

function provision_drupal_provision_update_validate() {
  _provision_drupal_url_required;
  _provision_drupal_valid_installed_site();
}

function provision_drupal_provision_update(&$data, $url) {
  provision_internal_init($data['site_url'], false);
  provision_platform_include(dirname(__FILE__), 'update');
}

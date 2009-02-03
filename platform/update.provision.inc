<?php

function provision_drupal_provision_update_validate() {
  _provision_drupal_url_required;
  _provision_drupal_valid_installed_site();
}

function provision_drupal_provision_update(&$data, $url) {
  $cmd = sprintf("php %s/drupal_update.php %s", dirname(__FILE__), $url);
  provision_exec($cmd, $data);
}



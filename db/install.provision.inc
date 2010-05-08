<?php

function drush_db_provision_install_validate() {
  provision_service('db')->connect();
}

function drush_db_pre_provision_install() {
  provision_service('db')->create_site_database();
}

function drush_db_pre_provision_install_rollback() {
  if (!_provision_drupal_site_installed()) {
    provision_service('db')->destroy_site_database();
  }
 
  
}

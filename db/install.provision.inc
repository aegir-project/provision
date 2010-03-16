<?php

function drush_db_provision_install_validate($url) {
  provision_service('db')->connect();
}

function drush_db_pre_provision_install($url) {
  provision_service('db')->create_site_database($url);
}

function drush_db_pre_provision_install_rollback($url) {
  if (!_provision_drupal_site_installed($url)) {
    provision_service('db')->destroy_site_database(
      drush_get_option('db_name'), drush_get_option('db_user'), 
      drush_get_option('db_passwd')
    );
  }
 
  
}

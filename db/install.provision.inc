<?php

function drush_db_provision_install_validate() {
  d()->service('db')->connect();
}

function drush_db_pre_provision_install() {
  d()->service('db')->create_site_database();
}

function drush_db_pre_provision_install_rollback() {
  d()->service('db')->destroy_site_database();
}

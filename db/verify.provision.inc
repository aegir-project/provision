<?php

function drush_db_provision_verify_validate() {
  if (d()->type === 'site') {
    d()->service('db')->connect();
  }
}

function drush_db_pre_verify_install() {
  if (d()->type === 'site') {
    d()->service('db')->create_site_database();
  }
}

function drush_db_pre_provision_verify_rollback() {
  if (d()->type === 'site') {
    d()->service('db')->destroy_site_database();
  }
}

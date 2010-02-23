<?php

function drush_provision_mysql_provision_server_validate() {
  provision_db_connect();
}

/**
 * Implementation of hook_provision_server
 *
 * Can't be rolled back.
 */
function drush_provision_mysql_provision_server() {
  if (!_provision_mysql_can_create_database()) {
    drush_set_error('PROVISION_CREATE_DB_FAILED');
  }
  else {
    drush_log(dt('Mysql can create new databases.'), 'message');
  }
}

function drush_provision_mysql_post_provision_server() {
  drush_set_option('master_db', drush_get_option('master_db'), 'user');
}


<?php

function provision_mysql_provision_verify_validate() {
  provision_db_connect();
}

/**
 * Implementation of hook_provision_verify
 *
 * Can't be rolled back.
 */
function provision_mysql_provision_verify() {
  if (PROVISION_CONTEXT_PLATFORM) {
    if (!_provision_mysql_can_create_database()) {
      drush_set_error('PROVISION_CREATE_DB_FAILED');
    }
    else {
      drush_log(dt('Mysql can create new databases.'), 'message');
    }
  }
}

function provision_mysql_provision_post_verify() {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('master_db', drush_get_option('master_db'), 'platform');
  }
}


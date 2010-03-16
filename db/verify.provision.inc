<?php

function drush_db_provision_verify_validate() {
  provision_service('db')->connect();
}

/**
 * Implementation of hook_provision_verify
 *
 * Can't be rolled back.
 */
function drush_db_provision_verify() {
  if (PROVISION_CONTEXT_PLATFORM) {
    if (!provision_service('db')->can_create_database()) {
      drush_set_error('PROVISION_CREATE_DB_FAILED');
    }
    else {
      drush_log(dt('Mysql can create new databases.'), 'message');
    }
  }
}

function drush_db_post_provision_verify() {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('master_db', drush_get_option('master_db'), 'drupal');
  }
}


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
      provision_set_error(PROVISION_DB_ERROR | PROVISION_FRAMEWORK_ERROR);
      provision_log('error', t('Unable to create new databases.'));
    }
    else {
      provision_log("message", t('Mysql can create new databases.'));
    }
  }
}


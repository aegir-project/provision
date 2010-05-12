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
  if (PROVISION_CONTEXT_SERVER) {
    provision_service('db')->verify();
  }
}

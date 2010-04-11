<?php


function drush_db_provision_delete_validate($url = null) {
  if (PROVISION_CONTEXT_SITE) {
    provision_service('db')->connect();
  }
}

/**
 * Implementation of hook_provision_delete()
 *
 * This will drop the database, revoke the privileges and flush the privileges.
 */
function drush_db_provision_delete($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    provision_service('db')->destroy_site_database($url);
  }
}

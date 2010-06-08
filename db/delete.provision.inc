<?php

function drush_db_provision_delete_validate() {
  if (d()->type === 'site') {
    d()->service('db')->connect();
  }
}

/**
 * Implementation of hook_provision_delete()
 *
 * This will drop the database, revoke the privileges and flush the privileges.
 */
function drush_db_provision_delete() {
  if (d()->type === 'site') {
    d()->service('db')->destroy_site_database();
  }
}

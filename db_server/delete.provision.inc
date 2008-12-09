<?php


function provision_mysql_provision_delete_validate() {
  provision_db_connect();
}

/**
 * Implementation of hook_provision_delete()
 *
 * This will drop the database, revoke the privileges and flush the privileges.
 */
function provision_mysql_provision_delete(&$data, $url = NULL) {
  return _provision_mysql_destroy_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
}


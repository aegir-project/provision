<?php


function provision_mysql_provision_delete_validate() {
  provision_db_connect();
}

/**
 * Implementation of hook_provision_delete()
 *
 * This will drop the database, revoke the privileges and flush the privileges.
 */
function provision_mysql_provision_delete($url = NULL) {
  return  _provision_mysql_destroy_site_db(
    drush_get_option('db_name'), drush_get_option('db_user'),
    drush_get_option('db_passwd'), drush_get_option('db_host'));
}


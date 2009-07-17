<?php
// $Id$

function drush_provision_mysql_provision_migrate_validate() {
  provision_db_connect();
}

// Deletes the old database credentials
function drush_provision_mysql_post_provision_migrate($url) {
  return  _provision_mysql_destroy_site_db(drush_get_option('db_name'), drush_get_option('db_user'), drush_get_option('db_passwd'));
}

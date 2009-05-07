<?php

function drush_provision_mysql_provision_install_validate() {
  provision_db_connect();
}

function drush_provision_mysql_pre_provision_install($url = NULL) {
  $db_type = drush_set_option('db_type', drush_get_option('db_type'), 'site');
  $db_host = drush_set_option('db_host', drush_get_option('db_host'), 'site');
  $db_passwd = drush_set_option('db_passwd', provision_password(), 'site');
  $db_name = drush_set_option('db_name', _provision_mysql_suggest_db_name($url), 'site');
  $db_user = drush_set_option('db_user', $db_name, 'site');

  // fetch the webserver name to create a GRANT specific to this server
  if ($db_client = drush_get_option('web_ip')) {
    // the IP isn't available, use the web host for the GRANT
    $db_client = drush_get_option('web_host');
  }
  _provision_mysql_new_site_db($db_name, $db_user, $db_passwd, $db_client);
}

function drush_provision_mysql_pre_provision_install_rollback($url = NULL) {
  _provision_mysql_destroy_site_db(
    drush_get_option('db_name'), drush_get_option('db_user'),
    drush_get_option('db_passwd'), drush_get_option('db_host')
  );
}

<?php

function drush_provision_mysql_provision_deploy_validate() {
  provision_db_connect();
}

function drush_provision_mysql_provision_deploy($url) {
  $db_type = drush_set_option('db_type', drush_get_option('db_type'), 'site');
  $db_host = drush_set_option('db_host', drush_get_option('db_host'), 'site');
  $db_passwd = drush_set_option('db_passwd', provision_password(), 'site');
  $db_name = drush_set_option('db_name', _provision_mysql_suggest_db_name($url), 'site');
  $db_user = drush_set_option('db_user', $db_name, 'site');

  _provision_mysql_new_site_db($db_name, $db_user, $db_passwd);

  _provision_mysql_import_dump(
    drush_get_option('sites_path') .'/'. $url .'/database.sql',
    $db_name, $db_user, $db_passwd, $db_host );
}


function drush_provision_mysql_provision_deploy_rollback($url = NULL) {
  _provision_mysql_destroy_site_db(drush_get_option('db_name'), drush_get_option('db_user'), drush_get_option('db_passwd'));
}

// Rollback doesn't apply here yet. Unless we trigger a deploy of the first dump
// made. Which could go on infinitely if something is really long.
function drush_provision_mysql_post_provision_deploy($url = NULL) {
  provision_path('unlink', drush_get_option('sites_path') .'/'. $url .'/database.sql', TRUE,
    dt("Removed dump file @path after restoring from it"),
    dt("Could not remove dump file @path"), DRUSH_PERM_ERROR);
}

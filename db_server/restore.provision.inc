<?php

function provision_mysql_provision_restore_validate() {
  provision_db_connect();
}

function provision_mysql_provision_pre_restore($url = NULL) {
  $db_type   = drush_set_option('db_type', drush_get_option('db_type'));
  $db_host   = drush_set_option('db_host', drush_get_option('db_host'));
  $db_passwd = drush_set_option('db_passwd', provision_password());
  $db_name   = drush_set_option('db_name', _provision_mysql_suggest_db_name($url));
  $db_user   = drush_set_option('db_user', $db_name);

  _provision_mysql_new_site_db($db_name, $db_user, $db_passwd, $db_host);
}

function provision_mysql_provision_restore($url) {
  _provision_mysql_import_dump(
    drush_get_option('sites_path') .'/'. $url .'/database.sql',
    drush_get_option('db_name'), drush_get_option('db_user'),
    drush_get_option('db_passwd'), drush_get_option('db_host')
  );
}


function provision_mysql_provision_pre_restore_rollback($url = NULL) {
  _provision_mysql_destroy_site_db(
    drush_get_option('db_name'), drush_get_option('db_user'),
    drush_get_option('db_passwd'), drush_get_option('db_host'));

  $keys = array('db_name', 'db_passwd', 'db_user', 'db_host');

  //Restore the original database credentials of the site.
  // They were never truly lost, but the options in the 'process' context
  // were overriding them. By unsetting the temporary options, the originals
  // are automatically restored.
  foreach ($keys as $key) {
    drush_unset_option($key, 'process');
  }
}

// Rollback doesn't apply here yet. Unless we trigger a restore of the first dump
// made. Which could go on infinitely if something is really long.
function provision_mysql_provision_post_restore($url = NULL) {
  provision_path('unlink', drush_get_option('sites_path') . '/' . $url .'/database.sql', TRUE,
    dt("Removed dump file @path after restoring from it"),
    dt("Could not remove dump file @path"), DRUSH_PERM_ERROR);

  // We have now completed successfully, remove the old database.
  _provision_mysql_destroy_site_db(
    drush_get_option('db_name', null, 'site'), drush_get_option('db_user', null, 'site'),
    drush_get_option('db_passwd', null, 'site'), drush_get_option('db_host', null, 'site')
  );

  // The new database credentials will be saved against the site now.
  drush_set_option('db_name', drush_get_option('db_name'), 'site');
  drush_set_option('db_type', drush_get_option('db_type'), 'site');
  drush_set_option('db_user', drush_get_option('db_user'), 'site');
  drush_set_option('db_host', drush_get_option('db_host'), 'site');
  drush_set_option('db_passwd', drush_get_option('db_passwd'), 'site');
}


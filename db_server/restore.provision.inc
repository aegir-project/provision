<?php

function provision_mysql_provision_restore_validate() {
  provision_db_connect();
}

function provision_mysql_provision_pre_restore(&$data, $url = NULL) {
  // store a backup of the credentials of the site.
  $data['old_db_name'] = $data['db_name'];
  $data['old_db_passwd'] = $data['db_passwd'];
  $data['old_db_user'] = $data['db_user'];
  $data['old_db_host'] = $data['db_host'];

  # generate a random password for use
  $data['db_passwd'] = provision_password(); 
  $data['db_name'] = _provision_mysql_suggest_db_name($data, $url);
  $data['db_user'] = $data['db_name'];

  _provision_mysql_new_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
}

function provision_mysql_provision_restore(&$data, $url) {
  _provision_mysql_import_dump(PROVISION_SITES_PATH .'/'. $url .'.restore/database.sql', $data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
}


function provision_mysql_provision_pre_restore_rollback(&$data, $url = NULL) {
  _provision_mysql_destroy_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);

  $keys = array('db_name', 'db_passwd', 'db_user', 'db_host');
  //Restore the original database credentials of the site.
  foreach ($keys as $key) {
    $data[$key] = $data['old_'. $key];
  }
}

// Rollback doesn't apply here yet. Unless we trigger a restore of the first dump
// made. Which could go on infinitely if something is really long.
function provision_mysql_provision_post_restore(&$data, $url = NULL) {
  provision_path('unlink', 'sites/'. $url .'/database.sql', TRUE,
    dt("Removed dump file @path after restoring from it"),
    dt("Could not remove dump file @path"), PROVISION_PERM_ERROR);
  _provision_mysql_destroy_site_db($data['old_db_name'], $data['old_db_user'], $data['old_db_passwd'], $data['old_db_host']);
}


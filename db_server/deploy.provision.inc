<?php

function provision_mysql_provision_deploy_validate() {
  provision_db_connect();
}

function provision_mysql_provision_deploy(&$data, $url) {
  # generate a random password for use
  $data['db_passwd'] = provision_password(); 
  $data['db_name'] = _provision_mysql_suggest_db_name($data, $url);
  $data['db_user'] = $data['db_name'];

  _provision_mysql_new_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);

  _provision_mysql_import_dump(PROVISION_SITES_PATH .'/'. $url .'/database.sql', $data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
}


function provision_mysql_provision_deploy_rollback(&$data, $url = NULL) {
  _provision_mysql_destroy_site_db($data['db_name'], $data['db_user'], $data['db_passwd'], $data['db_host']);
}

// Rollback doesn't apply here yet. Unless we trigger a deploy of the first dump
// made. Which could go on infinitely if something is really long.
function provision_mysql_provision_post_deploy(&$data, $url = NULL) {
  provision_path('unlink', 'sites/'. $url .'/database.sql', TRUE,
    dt("Removed dump file @path after restoring from it"),
    dt("Could not remove dump file @path"), DRUSH_PERM_ERROR);
}

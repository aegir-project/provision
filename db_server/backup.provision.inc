<?php

function provision_mysql_provision_pre_backup(&$data, $url = NULL) {
  drush_log("Generating mysql dump for $url.", 'backup');
  provision_shell_exec("mysqldump -u%s -p%s %s > sites/%s/database.sql", $data['db_user'], $data['db_passwd'], $data['db_name'], $url); 
}

function provision_mysql_provision_pre_backup_rollback(&$data, $url = NULL) {
  provision_path("unlink", PROVISION_SITES_PATH . "/$url/database.sql", TRUE, dt("Deleted mysql dump from sites directory"), 
  dt("Could not delete mysql dump from sites directory"));
}

function provision_mysql_provision_post_backup(&$data, $url = NULL) {
  provision_mysql_provision_pre_backup_rollback($data, $url);
}


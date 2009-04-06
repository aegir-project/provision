<?php

function provision_mysql_pre_provision_backup($url = NULL) {
  drush_log("Generating mysql dump for $url.", 'backup');
  provision_shell_exec("mysqldump -u%s -p%s %s > sites/%s/database.sql", drush_get_option('db_user'), drush_get_option('db_passwd'), drush_get_option('db_name'), $url); 
}

function provision_mysql_pre_provision_backup_rollback($url = NULL) {
  provision_path("unlink", drush_get_option('sites_path') . "/$url/database.sql", TRUE, dt("Deleted mysql dump from sites directory"), 
  dt("Could not delete mysql dump from sites directory"));
}

function provision_mysql_post_provision_backup($url = NULL) {
  provision_mysql_pre_provision_backup_rollback($url);
}


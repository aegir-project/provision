<?php

function drush_provision_mysql_pre_provision_backup($url = NULL) {
  drush_log("Generating mysql dump for $url.", 'backup');
  provision_shell_exec("mysqldump -h%s -u%s -p%s -rsites/%s/database.sql %s", drush_get_option('db_host'), drush_get_option('db_user'),drush_get_option('db_passwd'), $url, drush_get_option('db_name')); 
}

function drush_provision_mysql_pre_provision_backup_rollback($url = NULL) {
  provision_path("unlink", drush_get_option('sites_path') . "/$url/database.sql", TRUE, dt("Deleted mysql dump from sites directory"), 
  dt("Could not delete mysql dump from sites directory"));
}

function drush_provision_mysql_post_provision_backup($url = NULL) {
  drush_provision_mysql_pre_provision_backup_rollback($url);
}


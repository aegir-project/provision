<?php

function provision_mysql_provision_backup(&$data, $url = NULL) {
  provision_log("backup", "Generating mysql dump for $url.");
  provision_shell_exec("mysqldump -u%s -p%s %s > sites/%s/database.sql", $data['db_user'], $data['db_passwd'], $data['db_name'], $url); 
  provision_shell_exec("cd sites/%; tar -rf %s database.sql; rm database.sql", $url, $data['backup_file']);
}



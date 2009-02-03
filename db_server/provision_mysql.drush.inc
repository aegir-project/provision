<?php
// $Id$
/**
 * @file
 *    Mysql provisioning module.
 *
 * The goal of this module is to create mysql databases and user accounts, for sites that are about to be created.
 * It uses the provision API to tie into the right places in the site creation work flow.
 */
include_once('provision.mysql.inc');

function provision_mysql_provision_finalize() {
  provision_db_close();
}

/**
 * Generate a new mysql database and user account for the specified credentials
 */
function _provision_mysql_new_site_db($db_name, $db_user, $db_passwd, $db_host) {
  if (!_provision_mysql_create_database($db_name) ||
      !_provision_mysql_database_exists($db_name) ) {
   provision_set_error(PROVISION_DB_ERROR);
   provision_log("error", "Database could not be created.");
   return FALSE;
  }
  
  if (!_provision_mysql_grant($db_name, $db_user, $db_passwd)) {
    provision_log("warning", "Could not GRANT user access.");
  }
  _provision_mysql_flush_privileges();

  $status = _provision_mysql_database_exists($db_name);

  if ($status) {
    provision_log('success', dt('Created @name database', array("@name" => $db_name)));
  }
  else {
    provision_set_error(PROVISION_DB_ERROR);
    provision_log("error", dt("Could not create @name database", array("@name" => $db_name)));
  }
  return $status; 
   //TODO : Test to confirm that the database is actually writeable. Taking this on faith for now.
}

/**
 * Remove the database and user account for the supplied credentials
 */
function _provision_mysql_destroy_site_db($db_name, $db_user, $db_passwd, $db_host) {
    
  if ( _provision_mysql_database_exists($data['db_name']) ) {
    provision_log("notice", dt("Dropping database @dbname", array('@dbname' => $db_name)));
    if (!_provision_mysql_drop_database($db_name)) {
      provision_log("warning", dt("Failed to drop database @dbname", array('@dbname' => $db_name)));
    }
  }
  
  if ( _provision_mysql_database_exists($db_name) ) {
   provision_set_error(PROVISION_DB_ERROR);
   provision_log("error", "Database could not be destroyed.");
   return FALSE;
  }
  
  provision_log("notice", "Revoking privileges");
  _provision_mysql_flush_privileges();
  if (!_provision_mysql_revoke($db_name, $db_user)) {
      provision_log("warning", dt("Failed to revoke user privileges"));
  }
}


function _provision_mysql_database_exists($name) {
  return provision_db_result(provision_db_query("SHOW DATABASES LIKE '%s'", $name));
}

function _provision_mysql_drop_database($name) {
  return provision_db_query("DROP DATABASE `%s`", $name);
}

function _provision_mysql_create_database($name) {
  return provision_db_query("CREATE DATABASE %s", $name);  
}

function _provision_mysql_flush_privileges() {
  return provision_db_query("FLUSH PRIVILEGES");  
}

function _provision_mysql_can_create_database() {
  $test = 'provision_test';
  _provision_mysql_create_database($test);
  if (_provision_mysql_database_exists($test)) {
    if (!_provision_mysql_drop_database($test)) {
      provision_log("warning", dt("Failed to drop database @dbname", array('@dbname' => $test)));
    }
    return TRUE;
  }
  return FALSE;
}

function _provision_mysql_grant($name, $username, $password, $host = '') {
  $host = ($host) ? $host : '%';
  return provision_db_query("GRANT ALL PRIVILEGES ON `%s`.* TO `%s`@`%s` IDENTIFIED BY '%s'", $name, $username, $host, $password);
}

function _provision_mysql_revoke($name, $username, $host = '') {
  $host = ($host) ? $host : '%';
  return provision_db_query("REVOKE ALL PRIVILEGES ON `%s`.* FROM `%s`@`%s`", $name, $username, $host);
}

function _provision_mysql_import_dump($dump_file, $db_name, $db_user, $db_passwd, $db_host) {
  $exists = provision_path("exists", $dump_file, TRUE,
    dt('Found database dump at @path.'),
    dt('No database dump was found at @path.'),
    PROVISION_FRAMEWORK_ERROR);
  if ($exists) {
    $readable = provision_path("readable", $dump_file, TRUE, dt('Database dump at @path is readable'), 
      dt('The database dump at @path could not be read.'), 
      PROVISION_PERM_ERROR);
    if ($readable) {
      provision_log("notice", sprintf("Importing database using command: mysql -u%s -p%s -h%s %s < %s", 
        $db_user, $db_passwd, $db_host, $db_name, $dump_file));
      if (!provision_shell_exec("mysql -u%s -p%s -h%s %s < %s", $db_user, $db_passwd, $db_host, $db_name, $dump_file )) {
        provision_log("error", dt("Database import failed"));
      }
     }
   }
}

/**
 * Find a viable database name, based on available information.
 *
 * This function exists solely to work past mysql's database name restrictions.
 * As mysql also does not have the ability to rename databases, it is completely
 * possible that sites will be running with derivative names on the same server,
 * until the upgrade / restore process is completed.
 */ 
function _provision_mysql_suggest_db_name($data, $url) {
  if ($data['site_id']) {
    $id_suggest = 'site_'. $data['site_id'];
    $suggest[] = $id_suggest;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $id_suggest .'_'. $i;
    }
  }
  else {
    // This is a last option, and not ideal
    // Provision only users will trigger this mostly.
    $url_suggest = substr(0, 14, str_replace(array(".", "-"), '' , ereg_replace("^www\.", "", $url)));
    $suggest[] = $url_suggest;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $url_suggest .'_'. $i;
    }
  }

  foreach ($suggest as $option) {
    if (!_provision_mysql_database_exists($option)) {
      return $option;
    }
  }

}

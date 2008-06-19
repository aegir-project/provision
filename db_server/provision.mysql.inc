<?php
/**
 * @file mysql db api extension
 *
 * A collection of helper functions used by the main provision hooks to accomplish their tasks.
 */


/**
 * Generate a new mysql database and user account for the specified credentials
 */
function _provision_mysql_new_site_db($db_name, $db_user, $db_passwd, $db_host) {
  provision_set_active_db(_provision_master_db_url());
 
  _provision_mysql_create_database($db_name);
  
  if ( !_provision_mysql_database_exists($db_name) ) {
   provision_set_error(PROVISION_DB_ERROR);
   provision_log("error", "Database could not be created.");
   provision_set_active_db();
   return FALSE;
  }
  
  _provision_mysql_grant($db_name, $db_user, $db_passwd);
  _provision_mysql_grant($db_name, $db_user, $db_passwd, $db_host);

  if ($data['site_mysql_old_passwords']) {
    _provision_mysql_old_password($db_user, $db_passwd);
    _provision_mysql_old_password($db_user, $db_passwd, $db_host);
  }
  _provision_mysql_flush();
  provision_set_active_db();
   #TODO : Test to confirm that the database is actually writeable. Taking this on faith for now.
}

/**
 * Remove the database and user account for the supplied credentials
 */
function _provision_mysql_destroy_site_db($db_name, $db_user, $db_passwd, $db_host) {
  provision_set_active_db(_provision_master_db_url());
    
  if ( _provision_mysql_database_exists($data['site_db_name']) ) {
    provision_log("notice", t("Dropping database @dbname", array('@dbname' => $db_name)));
    _provision_mysql_drop_database($db_name);
  }
  
  if ( _provision_mysql_database_exists($db_name) ) {
   provision_set_error(PROVISION_DB_ERROR);
   provision_log("error", "Database could not be destroyed.");
   provision_set_active_db();
   return FALSE;
  }
  
  provision_log("notice", "Revoking privileges");
  _provision_mysql_revoke($db_name, $db_user);
  _provision_mysql_revoke($db_name, $db_user, $db_host);

  _provision_mysql_flush();
  provision_set_active_db();
}


function _provision_mysql_database_exists($name) {
  return db_result(db_query("SHOW DATABASES LIKE '%s'", $name));
}

function _provision_mysql_drop_database($name) {
  db_query("DROP DATABASE `%s`", $name);
}

function _provision_mysql_create_database($name) {
  db_query("CREATE DATABASE %s", $name);  
}

function _provision_mysql_can_create_database() {
  $test = 'provision_test';
  _provision_mysql_create_database($test);
  if (_provision_mysql_database_exists($test)) {
    _provision_mysql_drop_database($test);
    return true;
  }
  return false;
}

function _provision_mysql_grant($name, $username, $password, $host = '') {
  $host = ($host) ? $host : '%';
  db_query("GRANT ALL PRIVILEGES ON %s.* TO %s@`%s` IDENTIFIED BY '%s'", $name, $username, $host, $password);
}

function _provision_mysql_revoke($name, $username, $host = '') {
  $host = ($host) ? $host : '%';
  db_query("REVOKE ALL PRIVILEGES ON %s.* FROM %s@`%s`", $name, $username, $host);
}

function _provision_mysql_old_password($username, $password, $host = '') {
  $host = ($host) ? $host : '%';  
  db_query("SET PASSWORD FOR '%s'@'%s' = OLD_PASSWORD('%s')", $username, $host, $password);
}

function _provision_mysql_flush() {
  db_query("FLUSH PRIVILEGES");
}

function _provision_master_db_url($data = array()) {
  return sprintf("%s://%s:%s@%s/mysql", PROVISION_DB_TYPE, PROVISION_DB_USER, PROVISION_DB_PASSWD, PROVISION_DB_HOST);
}


function _provision_mysql_import_dump($dump_file, $db_name, $db_user, $db_passwd, $db_host) {
  $exists = provision_path("exists", $dump_file, true,
    t('Found database dump at @path.'),
    t('No database dump was found at @path.'),
    FILE_FRAMEWORK_ERROR);
  if ($exists) {
    $readable = provision_path("readable", $dump_file, true, '', 
      t('The database dump at @path could not be read.'), 
      PROVISION_PERM_ERROR);
    if ($readable) {
      provision_shell_exec("mysql -u%s -p%s -h%s %s < %s", 
        $db_user, $db_passwd, $db_host, $db_name, $dump_file );
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
function _provision_mysql_suggest_db_name($url, $data) {
  provision_set_active_db(_provision_master_db_url());
  if ($data['site_id']) {
    $id_suggest = 'site_' . $data['site_id'];
    $suggest[] = $id_suggest;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $id_suggest . '_' . $i;
    }
  }
  else {
    // This is a last option, and not ideal
    // Provision only users will trigger this mostly.
    $url_suggest = substr(0, 14, str_replace(array(".", "-"), '' , ereg_replace("^www\.", "", $url)));
    $suggest[] = $url_suggest;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $url_suggest . '_' . $i;
    }
  }

  foreach ($suggest as $option) {
    if (!_provision_mysql_database_exists($option)) {
      provision_set_active_db();
      return $option;
    }
  }

}

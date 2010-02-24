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

function provision_mysql_drush_init() {
  $command = drush_get_command();
  $command = explode(" ", $command['command']);
  if (preg_match("/^provision-/", $command[0])) {
    drush_set_default('master_db', $GLOBALS['db_url']);
    $master_db = drush_get_option('master_db');
    $db = parse_url($master_db);
    drush_set_default('master_db_user', urldecode($db['user']));
    drush_set_default('master_db_passwd', urldecode($db['pass']));

    drush_set_default('master_db_host', urldecode($db['host']));
    drush_set_default('db_host', urldecode($db['host']));

    drush_set_default('master_db_type', $db['scheme']);
    drush_set_default('db_type', $db['scheme']);
  }
}

function provision_mysql_drush_exit() {
  provision_db_close();
}

function provision_mysql_drush_help($section) {
  switch ($section) {
    case 'error:PROVISION_CREATE_DB_FAILED' :
      return dt('Unable to create new databases.');
    case 'error:PROVISION_DROP_DB_FAILED' :
      return dt('Unable to drop database.');
  }

}
/**
 * Generate a new mysql database and user account for the specified credentials
 */
function _provision_mysql_new_site_db($db_name, $db_user, $db_passwd, $db_grant_host = NULL) {
  if (is_null($db_grant_host)) {
    $db_grant_host = _provision_mysql_grant_host(
      drush_get_option('db_host', ''), 
      drush_get_option('web_ip', null), 
      drush_get_option('web_host', 'localhost'));
  }
  if (!_provision_mysql_create_database($db_name) ||
      !_provision_mysql_database_exists($db_name) ) {
   drush_set_error('PROVISION_CREATE_DB_FAILED');
   drush_log("Database could not be created.", 'error');
   return FALSE;
  }
  
  drush_log(dt("Granting privileges to %user@%client on %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));
  if (!_provision_mysql_grant($db_name, $db_user, $db_passwd, $db_grant_host)) {
    drush_log("Could not GRANT user access.", 'warning');
  }

  $status = _provision_mysql_database_exists($db_name);

  if ($status) {
    drush_log(dt('Created @name database', array("@name" => $db_name)), 'success');
  }
  else {
    drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not create @name database", array("@name" => $db_name)));
  }
  return $status; 
   //TODO : Test to confirm that the database is actually writeable. Taking this on faith for now.
}

/**
 * Remove the database and user account for the supplied credentials
 */
function _provision_mysql_destroy_site_db($db_name, $db_user, $db_passwd, $db_grant_host = NULL) {
  if (is_null($db_grant_host)) {
    $db_grant_host = _provision_mysql_grant_host(drush_get_option('db_host', ''), drush_get_option('web_ip'), drush_get_option('web_host'));
  }
  if ( _provision_mysql_database_exists($db_name) ) {
    drush_log(dt("Dropping database @dbname", array('@dbname' => $db_name)));
    if (!_provision_mysql_drop_database($db_name)) {
      drush_log(dt("Failed to drop database @dbname", array('@dbname' => $db_name)), 'warning');
    }
  }
  
  if ( _provision_mysql_database_exists($db_name) ) {
   drush_set_error('PROVISION_DROP_DB_FAILED');
   return FALSE;
  }
  
  drush_log(dt("Revoking privileges of %user@%client from %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));
  if (!_provision_mysql_revoke($db_name, $db_user, $db_grant_host)) {
      drush_log(dt("Failed to revoke user privileges"), 'warning');
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
  $test = drush_get_option('aegir_db_prefix', 'site_') .'test';
  _provision_mysql_create_database($test);
  if (_provision_mysql_database_exists($test)) {
    if (!_provision_mysql_drop_database($test)) {
      drush_log(dt("Failed to drop database @dbname", array('@dbname' => $test)), 'warning');
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
  $success = provision_db_query("REVOKE ALL PRIVILEGES ON `%s`.* FROM `%s`@`%s`", $name, $username, $host);
  
  // check if there are any privileges left for the user
  $grants = provision_db_query("SHOW GRANTS FOR `%s`@`%s`", $username, $host);
  $grant_found = FALSE;
  while ($grant = provision_db_fetch_array($grants)) {
    // those are empty grants: just the user line
    if (!preg_match("/^GRANT USAGE ON /", array_pop($grant))) {
      // real grant, we shouldn't remove the user
      $grant_found = TRUE;
      break;
    }
  }
  if (!$grant_found) {
    $success = provision_db_query("DROP USER `%s`@`%s`", $username, $host) && $success;
  }
  return $success;
}

function _provision_mysql_import_dump($dump_file, $db_name, $db_user, $db_passwd, $db_host) {
  $exists = provision_path("exists", $dump_file, TRUE,
    dt('Found database dump at @path.'),
    dt('No database dump was found at @path.'),
    'PROVISION_DB_DUMP_NOT_FOUND');
  if ($exists) {
    $readable = provision_path("readable", $dump_file, TRUE, dt('Database dump at @path is readable'), 
      dt('The database dump at @path could not be read.'), 
      'PROVISION_DB_DUMP_NOT_READABLE');
    if ($readable) {
      $cmd = sprintf("mysql --defaults-file=/dev/fd/3 %s", escapeshellcmd($db_name));
      drush_log(sprintf("Importing database using command: %s", $cmd));
      # pipe handling code, this is inspired by drush_provision_mysql_pre_provision_backup()
      # we go through all this trouble to hide the password from the commandline, it's the most secure way (apart from writing a temporary file, which would create conflicts in parallel runs)
     $mycnf = sprintf('[client]
host=%s
user=%s
password=%s
', $db_host, $db_user, $db_passwd);

      $descriptorspec = array(
        0 => array("file", $dump_file, "r"),
        1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
        2 => array("pipe", "w"),  // stderr is a file to write to
        3 => array("pipe", "r"),  // fd3 is our special file descriptor where we pass credentials
      );
      $process = proc_open($cmd, $descriptorspec, $pipes);
      $output = "";
      if (is_resource($process)) {
        fwrite($pipes[3], $mycnf);
        fclose($pipes[3]);
    
        $output = stream_get_contents($pipes[1]) . stream_get_contents($pipes[2]);
        // "It is important that you close any pipes before calling
        // proc_close in order to avoid a deadlock"
        fclose($pipes[1]);
        fclose($pipes[2]);
        $return_value = proc_close($process);
      } else {
        // XXX: failed to execute? unsure when this happens
        $return_value = -1;
      }

      if ($return_value != 0) {
        drush_set_error('PROVISION_DB_IMPORT_FAILED', dt("Database import failed: %output", array('%output' => $output)));
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
function _provision_mysql_suggest_db_name($url) {
  if ($sid = drush_get_option('site_id')) {
    $suggest_base = drush_get_option('aegir_db_prefix', 'site_') . $sid;
  }
  elseif ($name = drush_get_option('db_name')) {
    // consider the verified database name if no site id was provided
    //
    // we strip out eventual _N suffixes before finding a new db name
    // this is necessary because we may already have gone through this
    // process (in a migration) and had a _N suffix added
    $suggest_base = preg_replace('/_\d+$/', '', $name);
  }
  else {
    // This is a last option, and not ideal: base the db name on the
    // site name
    //
    // Provision only users will trigger this mostly.
    $suggest_base = substr(str_replace(array(".", "-"), '' , ereg_replace("^www\.", "", $url)), 0, 14);
  }
  $suggest[] = $suggest_base;
  for ($i = 0; $i < 100; $i++) {
    $suggest[] = $suggest_base .'_'. $i;
  }

  foreach ($suggest as $option) {
    if (!_provision_mysql_database_exists($option)) {
      return $option;
    }
  }

  drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not find a free database names after 100 attempts"));
  return false;

}

/**
 * Properly guess the host part of the MySQL username based on a given
 * database server host , web server IP and web server host.
 *
 * If the database and web server are the same machine, return a localhost
 * grant, which is a special and very common case which does not involve any
 * dns lookups and is thus faster and more secure.
 *
 * If the web server and database server are different machines, we prefer to
 * use the IP address, which is faster and more secure because fewer lookups
 * are done during connections.
 *
 */
function _provision_mysql_grant_host($db_host, $web_ip, $web_host) {
  // The database hostname is localhost, not defined or on the same ip/host as the webserver.
  if (in_array($db_host, array('127.0.0.1', 'localhost', '', $web_ip, $web_host))) {
    $grant = 'localhost';
  }
  // if we have the web ip, use that first.
  elseif ($web_ip) {
    $grant = $web_ip;
  } else {
    $grant = gethostbyname($web_host);
  }
  return $grant;
}

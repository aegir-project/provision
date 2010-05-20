<?php
// $Id$
/**
 * @file
 *    Mysql provisioning module.
 *
 * The goal of this module is to create mysql databases and user accounts, for sites that are about to be created.
 * It uses the provision API to tie into the right places in the site creation work flow.
 */

include_once(dirname(__FILE__) . '/../provision.service.inc');

function db_provision_services() {
  return array('db' => 'mysql');
}

function db_drush_exit() {
  // determine how to close it too.
  provision_service('db')->close();
}

function db_drush_help($section) {
  switch ($section) {
    case 'error:PROVISION_CREATE_DB_FAILED' :
      return dt('Unable to create new databases.');
    case 'error:PROVISION_DROP_DB_FAILED' :
      return dt('Unable to drop database.');
  }
}


class provisionService_db extends provisionService {

  function init() {
    $this->setProperty('master_db');
    $db = parse_url($this->master_db);

    $this->setProperty('master_db_user', urldecode($db['user']));
    $this->setProperty('master_db_passwd', urldecode($db['pass']));

    $this->setProperty('master_db_host', urldecode($db['host']));
    $this->setProperty('db_host', urldecode($db['host']));

    $this->setProperty('master_db_type', $db['scheme']);
    $this->setProperty('db_type', $db['scheme']);

    $creds = array();
    $options = array(
      'master_db_user' => 'user',
      'master_db_passwd' => 'pass',
      'master_db_host' => 'host',
      'master_db_type' => 'type');

    foreach ($options as $option => $key) {
      $creds[$key] = $this->$option;
    }
    $this->creds = $creds;
    return TRUE;
  }

  function verify() { 
    if (!$this->can_create_database()) {
      drush_set_error('PROVISION_CREATE_DB_FAILED');
    }
    else {
      drush_log(dt('Provision can create new databases.'), 'message');
    }
  }

  /**
   * Find a viable database name, based on available information.
   *
   * This function exists solely to work past mysql's database name restrictions.
   * As mysql also does not have the ability to rename databases, it is completely
   * possible that sites will be running with derivative names on the same server,
   * until the upgrade / restore process is completed.
   *
   * TODO: abstract this properly.
   */ 
  function suggest_db_name() {
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
      $suggest_base = substr(str_replace(array('.', '-'), '' , ereg_replace('^www\.', '', drush_get_option('uri'))), 0, 14);
    }
    $suggest[] = $suggest_base;
    for ($i = 0; $i < 100; $i++) {
      $suggest[] = $suggest_base .'_'. $i;
    }

    foreach ($suggest as $option) {
      if (!$this->database_exists($option)) {
        return $option;
      }
    }

    drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not find a free database names after 100 attempts"));
    return false;

  }


  /**
   * Generate a new mysql database and user account for the specified credentials
   */
  function create_site_database() {
    $creds = $this->generate_site_credentials();
    extract($creds);
    

    if (!$this->can_create_database()) {
     drush_set_error('PROVISION_CREATE_DB_FAILED');
     drush_log("Database could not be created.", 'error');
     return FALSE;
    }


    drush_log(dt("Granting privileges to %user@%client on %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));


    if (!$this->grant($db_name, $db_user, $db_passwd, $db_grant_host)) {
      drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not create database user @user", array('@user' => $db_user)));
    }

    $this->create_database($db_name);
    $status = $this->database_exists($db_name);

    if ($status) {
      drush_log(dt('Created @name database', array("@name" => $db_name)), 'success');
    }
    else {
      drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not create @name database", array("@name" => $db_name)));
    }
    return $status; 
  }


  
  /**
   * Remove the database and user account for the supplied credentials
   */
  function destroy_site_database() {
    $creds = $this->fetch_site_credentials();
    extract($creds);

    if ( $this->database_exists($db_name) ) {
      drush_log(dt("Dropping database @dbname", array('@dbname' => $db_name)));
      if (!$this->drop_database($db_name)) {
        drush_log(dt("Failed to drop database @dbname", array('@dbname' => $db_name)), 'warning');
      }
    }
    
    if ( $this->database_exists($db_name) ) {
     drush_set_error('PROVISION_DROP_DB_FAILED');
     return FALSE;
    }
    
    drush_log(dt("Revoking privileges of %user@%client from %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));
    if (!$this->revoke($db_name, $db_user, $db_grant_host)) {
      drush_log(dt("Failed to revoke user privileges"), 'warning');
    }
  }


  function import_site_database() {
    $dump_file = drush_get_option('sites_path') . '/' . drush_get_option('uri') . '/database.sql';

    $creds = $this->fetch_site_credentials();

    $exists = provision_service('file')->exists($dump_file)
      ->succeed('Found database dump at @path.')
      ->fail('No database dump was found at @path.', 'PROVISION_DB_DUMP_NOT_FOUND')
      ->status();
    if ($exists) {
      $readable = provision_service('file')->readable($dump_file)
        ->succeed('Database dump at @path is readable')
        ->fail('The database dump at @path could not be read.', 'PROVISION_DB_DUMP_NOT_READABLE')
        ->status();
      if ($readable) {
        $this->import_dump($dump_file, $creds);
      }
    }
  }

  function generate_site_credentials() {
    $creds = array();
    // replace with service type 
    $db_type = drush_get_option('db_type');
    // As of Drupal 7 there is no more mysqli type
    if (drush_drupal_major_version() >= 7) {
      $db_type = ($db_type == 'mysqli') ? 'mysql' : $db_type;
    }

    //TODO - this should not be here at all
    $creds['db_type'] = drush_set_option('db_type', $db_type, 'site');
    $creds['db_host'] = drush_set_option('db_host', drush_get_option('db_host'), 'site');
    $creds['db_passwd'] = drush_set_option('db_passwd', provision_password(), 'site');
    $creds['db_name'] = drush_set_option('db_name', $this->suggest_db_name(), 'site');
    $creds['db_user'] = drush_set_option('db_user', $creds['db_name'], 'site');
    $creds['db_grant_host'] = $this->grant_host();

    return $creds;
  }

  function fetch_site_credentials() {
    $creds = array();

    $keys = array('db_type', 'db_user', 'db_name', 'db_host', 'db_passwd');
    foreach ($keys as $key) {
      $creds[$key] = drush_get_option($key, '', 'site');
    }

    $creds['db_grant_host'] = $this->grant_host();
    return $creds;
  }

  function database_exists($name) {
    return FALSE;
  }

  function drop_database($name) {
    return FALSE;
  }

  function create_database($name) {
    return FALSE;
  }

  function can_create_database() { 
    return FALSE;
  }

  function grant($name, $username, $password, $host = '') {
    return FALSE;
  }

  function revoke($name, $username, $host = '') {
    return FALSE;
  }

  function import_dump($dump_file, $creds) {
    return FALSE;
  }

  function generate_dump() {
    return FALSE;
  }

  function grant_host() {
    return 'localhost';
  }

  /**
   * Write out server's drushrc alias file.
   */
  function write_alias() {
    return array(drush_get_option('provision-service-db') => array('provision-service-db', 'master_db'));
  }
}

/**
 * Indicates the place holders that should be replaced in _db_query_callback().
 */
define('PROVISION_QUERY_REGEXP', '/(%d|%s|%%|%f|%b)/');



// simple wrapper class for PDO based db services

class provisionService_db_pdo extends provisionService_db {
  public $conn;
  protected $creds;
  private $dsn;

  function init() {
    parent::init();
    $this->dsn = sprintf("%s:dbname=%s;host=%s", $this->PDO_type, $this->creds['name'], $this->creds['host']);
    $this->dsn = sprintf("%s:host=%s", $this->PDO_type,  $this->creds['host']);
  }

  function connect() {
    try {
      $this->conn = new PDO($this->dsn, $this->creds['user'], $this->creds['pass']);
    }
    catch (PDOException $e) {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', $e->getMessage());
    }
  } 

  function close() {
    $this->conn = null;
  }

  function query($query) {
    $args = func_get_args();
    array_shift($args);
    if (isset($args[0]) and is_array($args[0])) { // 'All arguments in one array' syntax
      $args = $args[0];
    }
    $this->query_callback($args, TRUE);
    $query = preg_replace_callback(PROVISION_QUERY_REGEXP, array($this, 'query_callback'), $query);
    
    try {
      $result = $this->conn->query($query);
    }
    catch (PDOException $e) {
      drush_log($e->getMessage(), 'warning');
      return FALSE;
    } 

    return $result;

  }

  function query_callback($match, $init = FALSE) {
    static $args = NULL;
    if ($init) {
      $args = $match;
      return;
    }

    switch ($match[1]) {
      case '%d': // We must use type casting to int to convert FALSE/NULL/(TRUE?)
        return (int) array_shift($args); // We don't need db_escape_string as numbers are db-safe
      case '%s':
        return substr($this->conn->quote(array_shift($args)), 1, -1);
      case '%%':
        return '%';
      case '%f':
        return (float) array_shift($args);
      case '%b': // binary data
        return $this->conn->quote(array_shift($args));
    }

  }
}


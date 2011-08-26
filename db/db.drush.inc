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
  return array('db' => NULL);
}

function db_drush_exit() {
  // determine how to close it too.
  d()->service('db')->close();
}

function db_drush_help($section) {
  switch ($section) {
    case 'error:PROVISION_CONNECT_DB_FAILED' :
      return dt('Unable to connect to database server.');
    case 'error:PROVISION_CREATE_DB_FAILED' :
      return dt('Unable to create new databases.');
    case 'error:PROVISION_CREATE_DB_USER_FAILED' :
      return dt('Unable to create new database users.');
    case 'error:PROVISION_DROP_DB_FAILED' :
      return dt('Unable to drop database.');
  }
}


class provisionService_db extends provisionService {
  protected $service = 'db';

  /**
   * Register the db handler for sites, based on the db_server option.
   */
  static function subscribe_site($context) {
    $context->setProperty('db_server', '@server_master');
    $context->is_oid('db_server');
    $context->service_subscribe('db', $context->db_server->name);
  }

  static function option_documentation() {
    return array(
      '--master_db' => 'server with db: Master database connection info, {type}://{user}:{password}@{host}',
    );
  }

  function init_server() {
    parent::init_server();
    $this->server->setProperty('master_db');
    $this->creds = array_map('urldecode', parse_url($this->server->master_db));

    return TRUE;
  }

  /**
   * Verifies database connection and commands
   */
  function verify_server_cmd() {
    if ($this->connect()) {
      if ($this->can_create_database()) {
        drush_log(dt('Provision can create new databases.'), 'message');
      }
      else {
        drush_set_error('PROVISION_CREATE_DB_FAILED');
      }
      if ($this->can_grant_privileges()) {
        drush_log(dt('Provision can create database users.'), 'message');;
      }
      else {
        drush_set_error('PROVISION_CREATE_DB_USER_FAILED');
      }
    } else {
      drush_set_error('PROVISION_CONNECT_DB_FAILED');
    }
  }

  /**
   * Find a viable database name, based on the site's uri.
   */ 
  function suggest_db_name() {
    $uri = $this->context->uri;

    $suggest_base = substr(str_replace(array('.', '-'), '' , preg_replace('/^www\./', '', $uri)), 0, 16);

    if (!$this->database_exists($suggest_base)) {
      return $suggest_base;
    }

    for ($i = 0; $i < 100; $i++) {
      $option = sprintf("%s_%d", substr($suggest_base, 0, 15 - strlen( (string) $i) ), $i);
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
  function create_site_database($creds = array()) {
    if (!sizeof($creds)) {
      $creds = $this->generate_site_credentials();
    }
    extract($creds);
    
    if (!$this->can_create_database()) {
      drush_set_error('PROVISION_CREATE_DB_FAILED');
      drush_log("Database could not be created.", 'error');
      return FALSE;
    }

    foreach ($this->grant_host_list() as $db_grant_host) {
      drush_log(dt("Granting privileges to %user@%client on %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));
      if (!$this->grant($db_name, $db_user, $db_passwd, $db_grant_host)) {
        drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not create database user @user", array('@user' => $db_user)));
      }
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
  function destroy_site_database($creds = array()) {
    if (!sizeof($creds)) {
      $creds = $this->fetch_site_credentials();
    }
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
    
    foreach ($this->grant_host_list() as $db_grant_host) {
      drush_log(dt("Revoking privileges of %user@%client from %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));
      if (!$this->revoke($db_name, $db_user, $db_grant_host)) {
        drush_log(dt("Failed to revoke user privileges"), 'warning');
      }
    }
  }


  function import_site_database($dump_file = null, $creds = array()) {
    if (is_null($dump_file)) {
      $dump_file = d()->site_path . '/database.sql';
    }

    if (!sizeof($creds)) {
      $creds = $this->fetch_site_credentials();
    }

    $exists = provision_file()->exists($dump_file)
      ->succeed('Found database dump at @path.')
      ->fail('No database dump was found at @path.', 'PROVISION_DB_DUMP_NOT_FOUND')
      ->status();
    if ($exists) {
      $readable = provision_file()->readable($dump_file)
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
    $db_type = drush_get_option('db_type', function_exists('mysqli_connect') ? 'mysqli' : 'mysql');
    // As of Drupal 7 there is no more mysqli type
    if (drush_drupal_major_version() >= 7) {
      $db_type = ($db_type == 'mysqli') ? 'mysql' : $db_type;
    }

    //TODO - this should not be here at all
    $creds['db_type'] = drush_set_option('db_type', $db_type, 'site');
    $creds['db_host'] = drush_set_option('db_host', $this->server->remote_host, 'site');
    $creds['db_port'] = drush_set_option('db_port', $this->server->db_port, 'site');
    $creds['db_passwd'] = drush_set_option('db_passwd', provision_password(), 'site');
    $creds['db_name'] = drush_set_option('db_name', $this->suggest_db_name(), 'site');
    $creds['db_user'] = drush_set_option('db_user', $creds['db_name'], 'site');

    return $creds;
  }

  function fetch_site_credentials() {
    $creds = array();

    $keys = array('db_type', 'db_port', 'db_user', 'db_name', 'db_host', 'db_passwd');
    foreach ($keys as $key) {
      $creds[$key] = drush_get_option($key, '', 'site');
    }

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

  /**
   * Return a list of hosts, as seen by the db server, which should be granted
   * access to the site database.
   */
  function grant_host_list() {
    return array_unique(array_map(array($this, 'grant_host'), $this->context->service('http')->grant_server_list()));
  }

  /**
   * Return a hostname suitable for database grants from a server object.
   */
  function grant_host(provisionContext_server $server) {
    return $server->remote_host;
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

  function init_server() {
    parent::init_server();
    $this->dsn = sprintf("%s:host=%s", $this->PDO_type,  $this->creds['host']);

    if ($this->has_port) {
      $this->dsn = "{$this->dsn};port={$this->server->db_port}";
    }
  }

  function connect() {
    try {
      $this->conn = new PDO($this->dsn, $this->creds['user'], $this->creds['pass']);
      return $this->conn;
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
  
  function database_exists($name) {
    $dsn = $this->dsn . ';dbname=' . $name;
    try {
      // Try to connect to the DB to test if it exists.
      $conn = new PDO($dsn, $this->creds['user'], $this->creds['pass']);
      // Free the $conn memory.
      $conn = NULL;
      return TRUE;
    }
    catch (PDOException $e) {
      return FALSE;
    }
  }
}


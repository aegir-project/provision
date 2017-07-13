<?php

class Provision_Service_db extends Provision_Service {
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
      'master_db' => 'server with db: Master database connection info, {type}://{user}:{password}@{host}',
      'db_grant_all_hosts' => 'Grant access to site database users from any web host. If set to TRUE, any host will be allowed to connect to MySQL site databases on this server using the generated username and password. If set to FALSE, web hosts will be granted access by their detected IP address.',
    );
  }

  function init_server() {
    parent::init_server();
    $this->server->setProperty('master_db');
    $this->server->setProperty('db_grant_all_hosts', FALSE);
    $this->server->setProperty('utf8mb4_is_supported', FALSE);
    $this->creds = array_map('urldecode', parse_url($this->server->master_db));

    return TRUE;
  }

  function save_server() {
    // Check database 4 byte UTF-8 support and save it for later.
    $this->server->utf8mb4_is_supported = $this->utf8mb4_is_supported();
  }

  /**
   * Verifies database connection and commands
   */
  function verify_server_cmd() {
    if ($this->connect()) {
      if ($this->can_create_database()) {
        drush_log(dt('Provision can create new databases.'), 'success');
      }
      else {
        drush_set_error('PROVISION_CREATE_DB_FAILED');
      }
      if ($this->can_grant_privileges()) {
        drush_log(dt('Provision can grant privileges on database users.'), 'success');;
      }
      else {
        drush_set_error('PROVISION_GRANT_DB_USER_FAILED');
      }
      if ($this->server->utf8mb4_is_supported) {
        drush_log(dt('Provision can activate multi-byte UTF-8 support on Drupal 7 sites.'), 'success');
      }
      else {
        drush_log(dt('Multi-byte UTF-8 for Drupal 7 is not supported on your system. See the <a href="@url">documentation on adding 4 byte UTF-8 support</a> for more information.', array('@url' => 'https://www.drupal.org/node/2754539')), 'warning');
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

    if (drush_get_error() || !$this->can_create_database()) {
      drush_set_error('PROVISION_CREATE_DB_FAILED');
      drush_log("Database could not be created.", 'error');
      return FALSE;
    }

    foreach ($this->grant_host_list() as $db_grant_host) {
      drush_log(dt("Granting privileges to %user@%client on %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)));
      if (!$this->grant($db_name, $db_user, $db_passwd, $db_grant_host)) {
        drush_set_error('PROVISION_CREATE_DB_FAILED', dt("Could not create database user @user", array('@user' => $db_user)));
      }
      drush_log(dt("Granted privileges to %user@%client on %database", array('%user' => $db_user, '%client' => $db_grant_host, '%database' => $db_name)), 'success');
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

  function can_grant_privileges() {
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
   * access to the site database. If server property 'db_grant_all_hosts' is
   * TRUE, use the MySQL wildcard '%' instead of
   */
  function grant_host_list() {
    if ($this->server->db_grant_all_hosts) {
      return array('%');
    }
    else {
      return array_unique(array_map(array($this, 'grant_host'), $this->context->service('http')->grant_server_list()));
    }
  }

  /**
   * Return a hostname suitable for database grants from a server object.
   */
  function grant_host(Provision_Context_server $server) {
    return $server->remote_host;
  }

  /**
   * Checks whether utf8mb4 support is available on the current database system.
   *
   * @return bool
   */
  function utf8mb4_is_supported() {
    // By default we assume that the database backend may not support 4 byte
    // UTF-8.
    return FALSE;
  }
}

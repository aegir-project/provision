<?php

// simple wrapper class for PDO based db services

class Provision_Service_db_pdo extends Provision_Service_db {
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
    $user = isset($this->creds['user']) ? $this->creds['user'] : '';
    $pass = isset($this->creds['pass']) ? $this->creds['pass'] : '';
    try {
      $this->conn = new PDO($this->dsn, $user, $pass);
      return $this->conn;
    }
    catch (PDOException $e) {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', $e->getMessage());
    }
  }
  
  function ensure_connected() {
    if (is_null($this->conn)) {
      $this->connect();
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
    $this->ensure_connected();
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


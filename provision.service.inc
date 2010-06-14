<?php

require_once('provision.inc');

require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';


class provisionService extends provisionChainedState {
  protected $server = '@server_localhost';
  public $context;


  function __get($name) {
    return d($this->server)->$name;
  }

  function __set($name, $value) {
    d($this->server)->$name = $value;
  }

  function __isset($name) {
    return isset(d($this->server)->$name);
  }

  function __unset($name) {
    unset(d($this->server)->$name);

  }

  function __construct($server) {
    $this->server = $server;
  }

  function sync($path = NULL, $exclude_sites = FALSE) {
    return d($this->server)->sync($path, $exclude_sites);
  }

  function verify() {
    return TRUE;
  }

  function init() {
    return TRUE;
  }

  function setProperty($field, $default = NULL, $array = FALSE) {
    d($this->server)->setProperty($field, $default, $array);
  }

  /**
   * Find the server environment this service is running on.
   *
   * @return
   *   A provisionServer object.
   */
  function server() {
    return d($this->server);
  }

  /**
   * Return service-specific configuration options for help.
   *
   * @return
   *   array('--option' => 'description')
   */
  static function option_documentation() {
    return array();
  }
}

class provisionService_null extends provisionService {

  function __get($name) {
    return false;
  }

  function __call($name, $args) {
    return false;
  }
}


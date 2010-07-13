<?php

require_once('provision.inc');

require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';


class provisionService extends provisionChainedState {
  protected $server = '@server_master';
  public $context;
  protected $service = null;

  protected $has_restart_cmd = FALSE;
  protected $has_port = FALSE;


  // All services have the ability to have an associated restart command and listen port.
  function init() {
    if (!is_null($this->service)) {
      if ($this->has_port) {
        $this->server->setProperty($this->service . '_port', $this->default_port());
      }
      if ($this->has_restart_cmd) {
        $this->server->setProperty($this->service . '_restart_cmd', $this->default_restart_cmd());
      }
    }
    return TRUE;
  }

  function default_port() {
    return false;
  }

  function default_restart_cmd() {
    return false;
  }



  function __construct($server) {
    $this->server = d($server);
  }

  function setContext($name) {
    $this->context = $name;
  }

  function sync($path = NULL, $additional_options = array()) {
    return $this->server->sync($path, $additional_options);
  }

  function verify() {
    return TRUE;
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

  /**
   * Null services do not synch files to the remote server,
   * because they have no associated config files.
   */
  function sync() {
    return null;
  }
}


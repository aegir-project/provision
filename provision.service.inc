<?php

require_once('provision.inc');

require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';


class provisionService extends provisionChainedState {
  protected $server = '@server_master';
  public $context;

  function __construct($server) {
    $this->server = d($server);
  }

  function setContext($name) {
    $this->context = $name;
  }

  function sync($path = NULL, $exclude_sites = FALSE) {
    return $this->server->sync($path, $exclude_sites);
  }

  function verify() {
    return TRUE;
  }

  function init() {
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


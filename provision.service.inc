<?php

require_once('provision.inc');

require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';


class provisionService extends provisionChainedState {
  protected $server = '@server_master';
  public $context;

  protected $service = null;
  protected $application_name = null;

  protected $has_restart_cmd = FALSE;
  protected $has_port = FALSE;

  protected $configs = array();

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

  /**
   * Generate a configuration file.
   *
   * This method will fetch the class to instantiate from the internal
   * $this->configs control array.
   */
  function create_config($config) {
    if (isset($this->configs[$config])) {
      // We cast to array here to silently accept the non array version
      foreach ((array) $this->configs[$config] as $class) {
        $config = new $class($this->context, $this->config_data($config));
        $config->write();
      }
    }
  }

  /**
   * Delete a configurationf file.
   * 
   * This method will fetch the class to instantiate from the internal
   * $this->configs control array.
   */
  function delete_config($config) {
    if (isset($this->configs[$config])) {
      // We cast to array here to silently accept the non array version
      foreach ((array) $this->configs[$config] as $class) {
        $config = new $class($this->context, $this->config_data($config, $class));
        $config->unlink();
      }
    }
  }

  /**
   * Fetch extra information the service wants to pass to he config file classes.
   */
  function config_data($config = null, $class = null) {
    $data = array();

    $data['server'] = $this->server;
    return $data;
  }


  /**
   * Restart the service using the provided restart command.
   */
  function restart() {
    // Only attempt to restart real services can have restart commands.
    if (!is_null($this->service) && $this->has_restart_cmd) {
      $service = (!is_null($this->application_name)) ? $this->application_name : $this->service;

      // Only attempt to restart if the command has been filled in.
      if ($cmd = $this->server->{"{$this->service}_restart_cmd"}) {
        if ($this->server->shell_exec($cmd)) {
          drush_log(dt('%service on %server has been restarted', array(
            '%service' => $service,
            '%server' => $this->server->remote_host))
          );

          return TRUE;
        }
        else {
          drush_log(dt('%service on %server could not be restarted.'.
            ' Changes might not be available until this has been done. (error: %msg)', array(
            '%service' => $service,
            '%server' => $this->server->remote_host, 
            '%msg' => join("\n", drush_shell_exec_output()))), 'warning');
        }
      }
    }
    return FALSE;
  }

  function __construct($server) {
    $this->server = is_object($server) ? $server : d($server);
  }

  function setContext($context) {
    $this->context = is_object($context) ? $context : d($context);
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


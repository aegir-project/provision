<?php
/**
 * @file
 * The base Provision service class.
 */

require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';


class Provision_Service extends Provision_ChainedState {

  /**
   * The server this service is associated to
   */
  protected $server = '@server_master';

  /**
   * The context in which this service stores its data
   *
   * This is usually an object made from a class derived from the
   * Provision_Context base class
   *
   * @see Provision_Context
   */
  public $context;

  protected $service = NULL;
  protected $application_name = NULL;

  protected $has_restart_cmd = FALSE;
  protected $has_port = FALSE;

  protected $configs = array();


  protected $config_cache = array();
  private $_config = array();


  /**
   * Implement the __call magic method.
   *
   * This implementation is really simple. It simply return NULL if the
   * method doesn't exist.
   *
   * This is used so that we can create methods for drush commands, and
   * can fail safely.
   */
  function __call($name, $args = array()) {
    return provision::method_invoke($this, $name, $args);
  }


  function init() {

  }

  // All services have the ability to have an associated restart command and listen port.
  function init_server() {
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

  function init_platform() {

  }

  function init_site() {

  }


  function default_port() {
    return false;
  }

  function default_restart_cmd() {
    return false;
  }

  /**
   * Set the currently active configuration object.
   *
   * @param $config
   *   String: Name of config file. The key to the $this->configs array.
   * @param $data
   *   Any optional information to be made available to templates. If a string, it will be
   *   turned into an array with the 'name' property the value of the string.
   */
  function config($config, $data = array()) {
    $this->_config = array();

    if (!isset($this->configs[$config])) {
      $service = (!is_null($this->application_name)) ? $this->application_name : $this->service;
      drush_log(dt('%service has no %name config file', array(
        '%service' => $service,
        '%name' => $config))
      );
      return $this;
    }

    if (!is_array($data) && is_string($data)) {
      $data = array('name' => $data);
    }

    if (!isset($this->config_cache[$this->context->name][$config])) {
      $this->config_cache[$this->context->name][$config] = array();
      foreach ((array) $this->configs[$config] as $class) {
        $this->config_cache[$this->context->name][$config][] = new $class($this->context, array_merge($this->config_data($config), $data));
      }
    }

    if (isset($this->config_cache[$this->context->name][$config])) {
      $this->_config = $this->config_cache[$this->context->name][$config];
    }

    return $this;
  }

  /**
   * Unlink the currently active config file.
   */
  function unlink() {
    foreach ($this->_config as $config) {
      if (is_object($config)) {
        $config->unlink();
      }
    }

    return $this;
  }

  /**
   * Write the currently active config file.
   */
  function write() {
    foreach ($this->_config as $config) {
      if (is_object($config)) {
        $config->write();
      }
    }

    return $this;
  }

  /**
   * Set a record on the data store of the currently active config file (if applicable).
   */
  function record_set($arg1, $arg2 = NULL) {
    foreach ($this->_config as $config) {
      if (is_object($config)) {
        if (is_object($config->store)) {
          if (is_array($arg1)) {
            $config->store->records = array_merge($config->store->records, $arg1);
          }
          elseif (!is_numeric($arg1)) {
            if (is_array($arg2)) {
              if (!isset($config->store->loaded_records[$arg1])
                  || !is_array($config->store->loaded_records[$arg1])) {
                $config->store->loaded_records[$arg1] = array();
              }
              if (!isset($config->store->records[$arg1])
                  || !is_array($config->store->records[$arg1])) {
                $config->store->records[$arg1] = array();
              }
              $config->store->records[$arg1] = array_merge($config->store->loaded_records[$arg1], $config->store->records[$arg1], $arg2);
            }
            else {
              $config->store->records[$arg1] = $arg2;
            }
          }
        }
      }
    }
    return $this;
  }

  /**
   * Delete a record from the data store of the currently active config file (if applicable).
   */
  function record_del($record) {
    return $this->record_set($record, NULL);
  }

  /**
   * Check if a record exists in the data store of the currently active config file (if applicable).
   */
  function record_exists($record) {
    foreach ($this->_config as $config) {
      if (is_object($config)) {
        if (is_object($config->store)) {
          return array_key_exists($record, $config->store->merged_records());
        }
      }
    }
    return FALSE;
  }

  /**
   * Fetch record(s) from the data store of the currently active config file (if applicable).
   */
  function record_get($key = NULL, $default = NULL) {
    foreach ($this->_config as $config) {
      if (is_object($config)) {
        if (is_object($config->store)) {
          $records = $config->store->merged_records();

          if (is_null($key)) {
            return $records;
          }

          if (isset($records[$key])) {
            return $records[$key];
          }
        }
      }
    }
    return $default;
  }


  /**
   * Generate a configuration file.
   *
   * This method will fetch the class to instantiate from the internal
   * $this->configs control array.
   */
  function create_config($config, $data = array()) {
    $this->config($config, $data)->write();
  }

  /**
   * Delete a configuration file.
   *
   * This method will fetch the class to instantiate from the internal
   * $this->configs control array.
   *
   * @return the return value of unlink(), which is usually the file object
   */
  function delete_config($config, $data = array()) {
    return $this->config($config, $data)->unlink();
  }

  /**
   * Fetch extra information the service wants to pass to the config file classes.
   */
  function config_data($config = NULL, $class = NULL) {
    $data = array();
    // Always pass the server this service is running on to configs.
    $data['server'] = $this->server;

    if (!is_null($this->application_name)) {
      // This value may be useful to standardize paths in config files.
      $data['application_name'] = $this->application_name;
    }
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
          drush_log(dt('%service on %server could not be restarted.' .
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

  /**
   * Set the currently active context of the service.
   *
   * @arg mixed $context
   *    the context to store this services data into. this can be an
   *    object, or a string in which case the object will be loaded
   *    dynamically with d()
   *
   * @see d()
   */
  function setContext($context) {
    $this->context = is_object($context) ? $context : d($context);
  }

  /**
   * Sync filesystem changes to the server hosting this service.
   */
  function sync($path = NULL, $additional_options = array()) {
    return $this->server->sync($path, $additional_options);
  }

  function fetch($path = NULL) {
    return $this->server->fetch($path);
  }

  function verify() {
    return TRUE;
  }

  /**
   * Return service-specific configuration options for help.
   *
   * @return
   *   array('option' => 'description')
   */
  static function option_documentation() {
    return array();
  }

  /**
   * Save symlink for this server from /var/aegir/config/APPLICATION_NAME.conf -> /var/aegir/config/SERVER/APPLICATION_NAME.conf
   */
  function symlink_service() {
    $file = $this->application_name . '.conf';
    // We link the app_name.conf file on the remote server to the right version.
    $cmd = sprintf('ln -sf %s %s',
      escapeshellarg($this->server->config_path . '/' . $file),
      escapeshellarg($this->server->aegir_root . '/config/' . $file)
    );

    if ($this->server->shell_exec($cmd)) {
      drush_log(dt("Created symlink for %file on %server", array(
        '%file' => $file,
        '%server' => $this->server->remote_host,
      )));
    };
  }
}

<?php

include_once('provision.config.inc');

class provisionService {
  protected $last_status;
  protected $tokens;

  function __construct() {
    $this->init();
  }

  function verify() {
    return TRUE;
  }

  function init() {
    return TRUE;
  }

  /**
   * Clear internal state
   */
  protected function _clear_state() {
    $this->last_status = NULL;
    $this->tokens = NULL;
  }

  /**
   * Return the status of the last operation.
   *
   * @return
   *   TRUE or FALSE for success or failure; NULL if there was not a previous
   *   operation.
   */
  function status() {
    return $this->last_status;
  }

  /**
   * Log a notice into the logging system, if the last operation completed
   * succesfully.
   * 
   * @param $message
   *   The message to log, a string.
   */
  function succeed($message) {
    if ($this->last_status === TRUE) {
      drush_log(dt($message, $this->tokens), 'message');      
    }

    return $this;
  }

  /**
   * Log a notice into the logging system, if the last operation did not
   * complete succesfully.
   * 
   * @param $message
   *   Log this as a error to the logging system, if the $error_codes parameter
   *   has been set, otherwise, log this as a warning. If the operation
   *   specifies an additional reason for the operation failing, it will be
   *   appended to this message.
   *
   * @param error_codes
   *   Generate these system level errors using the provision error bitmasks.
   */
  function fail($message, $error_codes = NULL) {
    if (!empty($this->tokens['@reason'])) {
      $message .= ' (@reason)';
    }
    if ($this->last_status === FALSE) {
      if (is_null($error_codes)) {
        // Trigger a warning
        drush_log(dt($message, $this->tokens), 'warning');
      }
      else {
        // Trigger a sysem halting error
        drush_set_error($error_codes, dt($message, $this->tokens));
      }
    }

    return $this;
  }

  function write_alias() {
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

provision_service('null', new provisionService_null());

/**
 * Retrieve and register provisionService objects. There should be at most one
 * of each service.
 *
 * @param $type
 *   The service type, like 'db', 'http', or 'file'. 'all' for an associative
 *   array of all services.
 * @param $_object
 *   Set the service type to $_object, an instance of a provisionService
 *   subclass.
 *
 * @return A provisionService object.
 */
function provision_service($type, $_object = NULL) {
  static $instances = null;

  if ($type === 'all') {
    return $instances;
  }

  if (is_object($_object)) {
    $instances[$type] = $_object;
  }

  if (isset($instances[$type])) {
    return $instances[$type];
  }

  return $instances['null'];
}

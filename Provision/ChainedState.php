<?php
/**
 * @file
 * The Provision_ChainedState class.
 */

/**
 * A base class for the service and file handling classes that implements
 * chaining of methods.
 */
class Provision_ChainedState {
  protected $last_status;
  protected $tokens;

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
      drush_log(dt($message, $this->tokens), 'success');
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
}

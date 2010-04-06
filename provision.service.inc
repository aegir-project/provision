<?php

class provisionService {
  protected $owner = '@self';
  protected $last_status;
  protected $tokens;

  function __get($name) {
    return d($this->owner)->$name;
  }

  function __set($name, $value) {
    d($this->owner)->$name = $value;
  }

  function __isset($name) {
    return isset(d($this->owner)->$name);
  }

  function __unset($name) {
    unset(d($this->owner)->$name);

  }

  function __construct($owner) {
    $this->owner = $owner;
  }

  function verify() {
    return TRUE;
  }

  function init() {
    return TRUE;
  }

  function setProperty($field, $default = null) {
    d($this->owner)->setProperty($field, $default);
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
}

class provisionConfig {
  public $template = null;
  public $data = array();
  public $description = null;
  public $owner = '@self';

  function __get($name) {
    return d($this->owner)->$name;
  }

  function __set($name, $value) {
    d($this->owner)->$name = $value;
  }

  function __isset($name) {
    return isset(d($this->owner)->$name);
  }

  function __unset($name) {
    unset(d($this->owner)->$name);

  }

  function __construct($owner) {
    $this->owner = $owner;
  }

  function process() {
    parent::process();
    return true;
  }

  function filename() {
    return false;
  }


  private function load_template() {
    $reflect = new reflectionObject($this);
    $base_dir = dirname($reflect->getFilename());

    if (isset($this->template)) {
      $file = $base_dir . '/' . $this->template;
      drush_log("Template loaded: $file");
      if (file_exists($file) && is_readable($file)) {
        return file_get_contents($file);
      }
    }
    return false;
  }


  private function render_template($template, $variables) {
    drush_errors_off();
    extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
    ob_start();                      // Start output buffering
    eval('?>'. $template);                 // Generate content
    $contents = ob_get_contents();   // Get the contents of the buffer
    ob_end_clean();                  // End buffering and discard
    drush_errors_on();
    return $contents;                // Return the contents
  }

  function write() {
    $filename = $this->filename();
    if ($filename && is_writeable(dirname($filename))) {
      // manipulate data before passing to template.
      $this->process();

      if ($template = $this->load_template()) {
        #todo - convert to file_service
        $file = fopen($filename, "w");
        $text = $this->render_template($template, $this->data);
        fwrite($file, $text);
        fclose($file);
        if ($this->description) {
          drush_log("Generated config : " . $this->description, 'success');
        }
      }
    }
  }

  function unlink() {
    return provision_service('file')->unlink($this->filename())->status();
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


function provision_service($type, $object = null) {
  return d()->$type;
}

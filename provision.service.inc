<?php

class provisionService {
  protected $templates = array();
  function verify() {
    return TRUE;
  }

  function templates() {
    $this->templates = array();
  }

  function render_config($template, $variables) {
    drush_errors_off();
    extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
    ob_start();                      // Start output buffering
    eval('?>'. $template);                 // Generate content
    $contents = ob_get_contents();   // Get the contents of the buffer
    ob_end_clean();                  // End buffering and discard
    drush_errors_on();
    return $contents;                // Return the contents
  }

  function write_config($file, $key, $extra_data = array()) {
    $data = drush_get_merged_options();
     
    if (is_writeable($file)) {
      if ($template = $this->load_template($key)) {
        $file = fopen($file, "w");
        $text = $this->render_config($template, $data);
        fwrite($file, $text);
        fclose($file);
      }
    }
  }
}

class provisionConfig {
  public $template = null;

  function __construct() {
    if (is_null($this->template)) {
      throw(exception);
    }
  }

  function filename() {
    return false;
  }

  function load() {
    $reflect = new reflectionObject($this);
    $base_dir = $reflect->getFilename();

    if (isset($this->templates[$key])) {
      $file = $base_dir . '/' . $this->templates[$key];

      if (file_exists($file) && is_readable($file)) {
        return file_get_contents($file);
      }
    }
    return false;
  }


  function render($template, $variables) {

  }

  function write_config($extra_data) {

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

function provision_service($type, $object = null) {
  static $instances = null;

  if (is_object($object)) {
    $instances[$type] = $object;
  }

  if (isset($instances[$type])) {
    return $instances[$type];
  }

  return $instances['null'];
}

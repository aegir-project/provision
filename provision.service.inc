<?php

class provisionService {

  function verify() {
    return TRUE;
  }

  function init() {
    return TRUE;
  }

  static function spawn($service, $default = null) {
    $reflect = new reflectionClass('provisionService_' . $service);
    $base_dir = dirname($reflect->getFilename());

    $type = drush_get_option($service . '-service-type', $default);

    
    if ($type) {
      $file = sprintf("%s/%s/%s_service.inc", $base_dir, $type, $type);
      $className = sprintf("provisionService_%s_%s", $service, $type);
      if (file_exists($file)) {
        drush_log("Loading $type driver for the $service service");
        include_once($file);
        provision_service($service, new $className())->init(); 
        return TRUE;
      }
    }

    return false;
  }

}

class provisionConfig {
  public $template = null;
  public $data = array();
  public $description = null;

  function __construct($data = array()) {
    if (is_null($this->template)) {
      throw(exception);
    }
    if (sizeof($data)) {
      $this->data = $data;
    }
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
    return provision_service('file')->path('unlink', $this->filename());
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

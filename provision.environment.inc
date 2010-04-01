<?php

function provision_environment_factory($options) {
  $classes = array('provisionServer', 'provisionPlatform', 'provisionSite');

  if (provisionServer::evaluate($options)) {
    $classname = array_shift($classes);
    if (provisionPlatform::evaluate($options)) {
      $classname = array_shift($classes);
      if (provisionSite::evaluate($options)) {
        $classname = array_shift($classes);
      }
    }
  }

  $object = new $classname($options);
  $object->init();
  $object->clean();
  return $object;
}

function & d($short_name = '@self', $object = null) {
  static $instances = null;

  if (is_object($object)) {
    $instances[$short_name] = $object;
  }

  if (isset($instances[$short_name])) {
    return $instances[$short_name];
  }

  return $instances['@self'];
}


class provisionEnvironment {
  protected $options = array();
  protected $properties = array();
  protected $map = array();
  protected $short_name = null;
  protected $type = null;

  function __get($name) {
    if (array_key_exists($name, $this->properties)) {
      return $this->properties[$name];
    }
  }

  function __set($name, $value) {
    if (!property_exists($this, $name)) {
      $this->properties[$name] = $value;
      $this->map[$name] = (array_key_exists($name, $this->map)) ? $this->map[$name] : $this->type;
    }
    else {
      $this->$name = $value;
    }
  }

  function __isset($name) {
    if (property_exists($this->properties, $name) || property_exists($this, $name)) {
      return TRUE;
    }
    return FALSE;
  }

  function __unset($name) {
    if (property_exists($this->properties, $name)) {
      unset($this->properties[$name]);
    }
    elseif (property_exists($this, $name)) {
      unset($this->$name);
    }

  }

  function __construct($options) {
    $this->options = $options;
  }

  function init() {
    return true;
  }

  function clean() {
    $this->options = array();
  }

  function setProperty($field, $default = null) {
    if (isset($this->options[$field])) {
      $this->$field = $this->options[$field];
    }
    else {
      $this->$field = $default;
    }
  }

  function setMap($field, $context) {
    $this->map[$field] = $context;
  }
}

class provisionServer extends provisionEnvironment {
  public static function evaluate($options) {
    return TRUE;
  }

  function __construct($options) {
    parent::__construct($options);
    $this->type = 'server';
    $this->setProperty('remote_host', 'localhost');
    $this->setProperty('script_user', get_current_user());
    $this->setProperty('aegir_root', '/var/aegir');
    $this->setProperty('backup_path', $this->aegir_root . '/backup');

    $this->setProperty('config_path', $this->aegir_root . '/config');
    $this->setProperty('alias_path', $this->config_path . '/drush.d');
  }

  function init() {
    parent::init();
    $this->load_services();
  }

  function load_services() {
    $service_list = drush_command_invoke_all('provision_services');
    foreach ($service_list as $service => $default) {
      $this->spawn_service($service, $default);
    }
  }


  function spawn_service($service, $default = null) {
    $reflect = new reflectionClass('provisionService_' . $service);
    $base_dir = dirname($reflect->getFilename());

    $type_option = "$service-service-type";
    
    $type = isset($this->options[$type_option]) ? $this->options[$type_option] : $default;
    if ($type) {
      $file = sprintf("%s/%s/%s_service.inc", $base_dir, $type, $type);
      $className = sprintf("provisionService_%s_%s", $service, $type);
      if (file_exists($file)) {
        drush_log("Loading $type driver for the $service service");
        include_once($file);
        $object = new $className();
        $object->init($this->options);
        $this->$service = $object;
      }
    }
    else {
      $this->$service = new provisionService_null();
    }
  }
}

class provisionPlatform extends provisionServer {
  public static function evaluate($options) {
    if ($options['root']) {
      return true;
    }
  }

  function __construct($options) {
    parent::__construct($options);
    $this->type = 'platform';
    $this->setProperty('root', $_SERVER['PWD']);
  }
}

class provisionSite extends provisionPlatform {

  public static function evaluate($options) {
    if ($options['uri']) {
      return true;
    }
  }

  function __construct($options) {
    parent::__construct($options);
    $this->type = 'site';
    $this->setProperty('uri');
    $this->setProperty('site_url', str_replace(array('https://', 'http://'), '', $this->uri));
    $this->setProperty('profile', 'default');
    $this->setProperty('language', 'en');
    $this->setProperty('aliases', array());

    // todo - turn into a re-usable mechanism for comma separated values
    if ($this->options['aliases'] && !is_array($site_data['aliases'])) {
      $this->aliases = explode(",", $site_data['aliases']);
    }
    else {
      $this->aliases = array();
    }

  }
}



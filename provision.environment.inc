<?php

function provision_environment_factory($name, $options, $root_object = FALSE) {
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

  d($name,  new $classname($name, $options), $root_object);
  d($name)->init();
  d($name)->clean();
  return d($name);
}

function & d($name = '@self', $object = null, $root_object = FALSE) {
  static $instances = null;
  static $default_instance = '@self';

  if ($name) {
    if ($root_object) {
      $default_instance = $name;
    }

    if (is_object($object)) {
      $instances[$name] = $object;
    }

    if (isset($instances[$name])) {
      return $instances[$name];
    }
  }

  return $instances[$default_instance];
}


class provisionEnvironment {
  protected $options = array();
  protected $properties = array();
  protected $map = array();
  protected $name = null;
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

  function __construct($name, $options) {
    $this->name = $name;
    $this->options = $options;
    unset($this->options['packages']);
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

  function getProperties() {
    return $this->properties;
  }
}

class provisionServer extends provisionEnvironment {
  public static function evaluate($options) {
    return TRUE;
  }

  function init() {
    parent::init();

    print_r($this->options);
    $this->type = 'server';
    $this->setProperty('remote_host', 'localhost');
    $this->setProperty('script_user', get_current_user());
    $this->setProperty('aegir_root', '/var/aegir');
    $this->setProperty('backup_path', $this->aegir_root . '/backup');

    $this->setProperty('config_path', $this->aegir_root . '/config');
    $this->setProperty('alias_path', $this->config_path . '/drush.d');
    $this->setProperty('master_url');
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
        $object = new $className($this->name);
        $object->init();
        $this->services[$service] = $object;
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

  function init() {
    parent::init();
    $this->type = 'platform';
    $this->setProperty('root', $_SERVER['PWD']);

    // publish path can really be removed ...
    $this->setProperty('publish_path', $this->root);

    $this->setProperty('sites_path', $this->root . '/sites');
  }
}

class provisionSite extends provisionPlatform {

  public static function evaluate($options) {
    if ($options['uri']) {
      return true;
    }
  }

  function init() {
    parent::init();
    $this->type = 'site';
    $this->setProperty('uri');
    $this->setProperty('site_url', str_replace(array('https://', 'http://'), '', $this->uri));
    $this->setProperty('language', 'en');
    $this->setProperty('aliases', array());
    $this->setProperty('site_port', 80);

    $this->setProperty('client_email');

    // todo - turn into a re-usable mechanism for comma separated values
    if ($this->options['aliases'] && !is_array($site_data['aliases'])) {
      $this->aliases = explode(",", $site_data['aliases']);
    }
    else {
      $this->aliases = array();
    }


    // this can potentially be handled by a drupal sub class
    $this->setProperty('profile', 'default');

  }
}



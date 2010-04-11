<?php
/**
 * Provision environment API
 *
 *
 */

/**
 * Store and access environment objects by their alias name
 *
 */
function & d($name = '@self', $root_object = FALSE) {
  static $instances = null;
  static $default_instance = '@self';

  if ($name) {
    if ($root_object) {
      $default_instance = $name;
    }

    if (isset($instances[$name])) {
      return $instances[$name];
    }
    else {
      $instances[$name] = provision_environment_factory($name);
      $instances[$name]->init();
      return $instances[$name];
    }
  }

  return $instances[$default_instance];
}

/**
 * Create a new environment object and cache it in the d() static cache function.
 */
function provision_environment_factory($name) {
  $classes = array('provisionServer', 'provisionPlatform', 'provisionSite');

  $options = drush_sitealias_get_record($name);

  if (provisionServer::evaluate($options)) {
    $classname = array_shift($classes);
    if (provisionPlatform::evaluate($options)) {
      $classname = array_shift($classes);
      if (provisionSite::evaluate($options)) {
        $classname = array_shift($classes);
      }
    }
  }

  return new $classname($name);
}


/**
 * Base environment class.
 *
 * Contains magic getter/setter functions
 */
class provisionEnvironment {
  protected $name = null;
  protected $properties = array();

  protected $type = null;
  protected $oid_map = array();

  protected $map = array();

  /**
   * Retrieve value from $properties array if property does not exist in class proper.
   *
   * TODO: consider returning a reference to the value, so we can do things like:
   *       `$this->options['option'] = 'value'` and it will correctly set it in the
   *       drush context cache.
   */
  function __get($name) {
    if ($name == 'options') {
      return drush_sitealias_get_record($this->name);
    }
    if (array_key_exists($name, $this->properties)) {
      if (isset($this->oid_map[$name]) && !empty($this->properties[$name])) {
        return d($this->properties[$name]);
      }
      else {
        return $this->properties[$name];
      }
    }
  }

  function is_oid($name) {
    $this->oid_map[$name] = TRUE;
  }

  /**
   * Store value in properties array if the property does not exist in class proper.
   *
   * Keep a map of what the $this->type is when the value is set, so we can retrieve
   * specific values later.
   */
  function __set($name, $value) {
    if (!property_exists($this, $name)) {
      $this->properties[$name] = $value;
      $this->map[$name] = (array_key_exists($name, $this->map)) ? $this->map[$name] : $this->type;
    }
    else {
      $this->$name = $value;
    }
  }

  /**
   * Check the properties array if the property does not exist in the class proper.
   */
  function __isset($name) {
    if (property_exists($this->properties, $name) || property_exists($this, $name)) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Remove the value from the properties array if the property does not exist in the class proper.
   */
  function __unset($name) {
    if (property_exists($this->properties, $name)) {
      unset($this->properties[$name]);
    }
    elseif (property_exists($this, $name)) {
      unset($this->$name);
    }

  }

  /**
   * Constructor for the environment.
   */
  function __construct($name) {
    $this->name = $name;

    if ($this->options['parent']) {
      $this->parent = $this->options['parent'];
      d($this->options['parent']);
    }
  }

  /**
   * Init stub function/
   */
  function init() {
    return true;
  }

  /**
   * Check the $options property for a field, saving to the properties array.
   */
  function setProperty($field, $default = null) {
    if (isset($this->options[$field])) {
      $this->$field = $this->options[$field];
    }
    else {
      $this->$field = $default;
    }
  }

  /**
   * Alter the map array to move a value between stacks.
   */
  function setMap($field, $context) {
    $this->map[$field] = $context;
  }

}

/**
 * Server environment class.
 *
 * This class bootstraps the Service API by generating server
 * objects for each of the available service types.
 */
class provisionServer extends provisionEnvironment {
  protected $services = array();
  public static function evaluate($options) {
    return TRUE;
  }

  function init() {
    parent::init();

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

  /**
   * Iterate through the available service types and spawn a handler for each type.
   */
  function load_services() {
    $service_list = drush_command_invoke_all('provision_services');
    foreach ($service_list as $service => $default) {
      $this->spawn_service($service, $default);
    }
  }

  /**
   * Spawn an instance for a specific service type and associate it to the owner.
   */
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
      }
    }
    else {
      $this->services[$service] = new provisionService_null();
    }
  }

  /**
   * Retrieve a service of a specific type from the environment.
   */
  function service($service) {
    return $this->services[$service];
  }
}

/**
 * Class for the platform environment.
 */
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

    $this->setProperty('web_server', $this->name);
    $this->is_oid('web_server');

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

    $this->setProperty('db_server', $this->parent->parent->name);
    $this->is_oid('db_server');


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



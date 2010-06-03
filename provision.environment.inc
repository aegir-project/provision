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
function & d($name = NULL, $root_object = FALSE) {
  static $instances = null;
  static $default_instance = '@self';

  if (is_null($name)) {
    return $instances[$default_instance];
  }
  else {
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
}

/**
 * Create a new environment object and cache it in the d() static cache function.
 */
function provision_environment_factory($name) {
  $classes = array('server' => 'provisionServer', 'platform' => 'provisionPlatform', 'site' => 'provisionSite');

  $options = drush_sitealias_get_record($name);
  $type = isset($options['context_type']) ? $options['context_type'] : 'server';
  $classname = $classes[$type];

  return new $classname($name);
}


/**
 * Base environment class.
 *
 * Contains magic getter/setter functions
 */
class provisionEnvironment {
  public $name = null;
  public $type = null;

  protected $properties = array();
  protected $parent_key = null;

  protected $oid_map = array();

  /**
   * Retrieve value from $properties array if property does not exist in class proper.
   *
   * TODO: consider returning a reference to the value, so we can do things like:
   *       `$this->options['option'] = 'value'` and it will correctly set it in the
   *       drush context cache.
   */
  function __get($name) {
    if ($name == 'options') {
      return array_merge(drush_sitealias_get_record($this->name), array_filter(drush_get_context('stdin')), array_filter(drush_get_context('options')));
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

  /**
   * Specify that a property contains a named context.
   */
  function is_oid($name) {
    $this->oid_map[$name] = TRUE;
  }

  /**
   * Store value in properties array if the property does not exist in class proper.
   */
  function __set($name, $value) {
    if (!property_exists($this, $name)) {
      $this->properties[$name] = $value;
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
  }

  /**
   * Init stub function/
   */
  function init() {
    $this->setProperty('context_type', 'server');
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
   * Write out this named context to an alias file.
   */
  function write_alias() {
    $config = new provisionConfig_drushrc_alias($this->name, $this->properties);
    $config->write();
  }

  function service($service, $name = null) {
    if (!is_null($this->parent_key)) {
      return $this->{$this->parent_key}->service($service, ($name) ? $name : $this->name);
    }
    else {
      return new provisionService_null($this->name);
    }
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

  static function option_documentation() {
    $options = array(
      '--remote_host' => 'server: host name; default localhost',
      '--script_user' => 'server: OS user name; default current user',
      '--aegir_root' => 'server: Aegir root; default /var/aegir',
      '--backup_path' => 'server: Aegir backup path; default {aegir_root}/backup',
      '--config_path' => 'server: Aegir configuration path; default {aegir_root}/config',
      '--master_url' => 'server: Hostmaster URL',
    );
    foreach (drush_command_invoke_all('provision_services') as $service => $default) {
      $reflect = new reflectionClass('provisionService_' . $service);
      $base_dir = dirname($reflect->getFilename());
      $types = array();
      $options['--' . $service . '-service-type'] = 'placeholder';
      foreach (array_keys(drush_scan_directory($base_dir, '%.*_service\.inc%')) as $service_file) {
        if (preg_match('%^' . $base_dir . '/([a-z]+)/(?:\1)_service.inc$%', $service_file, $match)) {
          $types[] = $match[1];
          include_once($service_file);
          $options = array_merge($options, call_user_func(array(sprintf('provisionService_%s_%s', $service, $match[1]), 'option_documentation')));
        }
      }
      $options['--' . $service . '-service-type'] = 'server: ' . implode(', ', $types) . ', or null; default ' . (empty($default) ? 'null' : $default);
    }
    return $options;
  }

  function init() {
    parent::init();

    $this->type = 'server';
    $this->setProperty('remote_host', 'localhost');
    $this->setProperty('script_user', get_current_user());
    $this->setProperty('aegir_root', '/var/aegir');
    $this->setProperty('backup_path', $this->aegir_root . '/backups');
    $this->setProperty('config_path', $this->aegir_root . '/config');
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
        $this->setProperty($type_option, $type);
      }
    }
    else {
      $this->services[$service] = new provisionService_null($this->name);
    }
  }

  /**
   * Retrieve a service of a specific type from the environment.
   */
  function service($service, $name = null) {
    $this->services[$service]->context = ($name) ? $name : $this->name;
    return $this->services[$service];
  }

  /**
   * Call method $callback on each of the environment's service objects.
   *
   * @param $callback
   *   A provisionService method.
   * @return
   *   An array of return values from method implementations.
   */
  function services_invoke($callback) {
    foreach (array_keys($this->services) as $service) {
      $this->service($service)->$callback();
    }
  }

  function verify() {
    $this->services_invoke('verify');
  }
}

/**
 * Class for the platform environment.
 */
class provisionPlatform extends provisionEnvironment {
  static function option_documentation() {
    return array(
      '--root' => 'platform: path to a Drupal installation',
      '--server' => 'drush backend server; default @server_localhost',
      '--web_server' => 'web server hosting the platform; default @server_localhost',
    );
  }

  function init() {
    $this->parent_key = 'server';

    parent::init();
    $this->type = 'platform';
    $this->setProperty('root');

    $this->setProperty('server', '@server_localhost');
    $this->is_oid('server');

    $this->setProperty('web_server', '@server_localhost');
    $this->is_oid('web_server');
  }

  function verify() {
    $this->service('http')->verify();
  }
}

class provisionSite extends provisionEnvironment {
  static function option_documentation() {
    return array(
      //todo
    );
  }

  function init() {
    $this->parent_key = 'platform';

    parent::init();
    $this->type = 'site';
    
    $this->setProperty('platform');
    $this->is_oid('platform');

    // we need to set the alias root to the platform root, otherwise drush will cause problems.
    $this->root = $this->platform->root;

    $this->setProperty('uri');
    $this->setProperty('language', 'en');
    $this->setProperty('aliases', array());
    $this->setProperty('site_port', 80);

    $this->setProperty('client_email');

    $this->setProperty('db_server');
    $this->is_oid('db_server');

    // todo - turn into a re-usable mechanism for comma separated values
    if ($this->options['aliases'] && !is_array($site_data['aliases'])) {
      $this->aliases = explode(",", $site_data['aliases']);
    }
    else {
      $this->aliases = array();
    }

    // this can potentially be handled by a Drupal sub class
    $this->setProperty('profile', 'default');
  }

  function verify() {
    $this->service('db')->verify();
    $this->service('http')->verify();
  }
}

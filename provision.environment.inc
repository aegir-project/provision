<?php
/**
 * @file Provision environment API
 */

/**
 * Store and access environment objects by alias name.
 *
 * @param $name
 *   A Drush alias name, including leading @.
 * @param $_root_object
 *   Internal use only, set default object returned by d().
 *
 * @return
 *   provisionEnvironment object.
 */
function & d($name = NULL, $_root_object = FALSE) {
  static $instances = null;
  static $default_instance = '@self';

  if (is_object($name)) {
    return $name;
  }

  if ($name == 'all') {
    return $instances;
  }

  if (is_null($name)) {
    return $instances[$default_instance];
  }
  else {
    if ($_root_object) {
      $default_instance = $name;
    }

    if (isset($instances[$name])) {
      return $instances[$name];
    }
    else {
      $instances[$name] = provision_environment_factory($name);
      $instances[$name]->method_invoke('init');
      $instances[$name]->type_invoke('init');

      return $instances[$name];
    }
  }
}

/**
 * Simple access layer for drush_sitealias_get_record.
 *
 * Everytime sitealiases are fetched a lot of processing happens, but if the
 * file doesnt exist yet there's a whole lot of unnecesary stuff happening. 
 *
 * We cache the result locally here.
 */
function provision_sitealias_get_record($name) {
  static $cache = array();

  if (!isset($cache[$name])) {
    $cache[$name] = drush_sitealias_get_record($name);
  }

  return $cache[$name];
}

/**
 * Create a new environment object and cache it in the d() static cache
 * function.
 */
function provision_environment_factory($name) {
  $classes = array('server', 'platform', 'site');

  $type = 'server';

  $record = provision_sitealias_get_record($name);
  $options = array_merge(drush_get_context('stdin'), drush_get_context('options'));

  if (isset($record['context_type'])) {
    $type = $record['context_type'];
  }
  elseif (isset($options['context_type'])) {
    $type = $options['context_type'];
  }
  $classname = "provisionEnvironment_{$type}";

  return new $classname($name);
}

/**
 * Base environment class.
 *
 * Contains magic getter/setter functions
 */
class provisionEnvironment {
  /**
   * Name for saving aliases and referencing.
   */
  public $name = null;

  /**
   * 'server', 'platform', or 'site'.
   */
  public $type = null;

  /**
   * Properties that will be persisted by provision-save. Access as object
   * members, $envoronment->property_name. __get() and __set handle this. In
   * init(), set defaults with setProperty().
   */
  protected $properties = array();

  /**
   * Keeps track of properites that are names of provisionEnvironment objects.
   * Set with is_oid().
   */
  protected $oid_map = array();

  protected $service_subs = array();
  protected $parent_key = null;

  /**
   * Retrieve value from $properties array if property does not exist in class
   * proper. Properties that refer to provisionEnvironment objects will be run
   * through d(), see is_oid().
   *
   * TODO: consider returning a reference to the value, so we can do things like:
   *       `$this->options['option'] = 'value'` and it will correctly set it in the
   *       drush context cache.
   */
  function __get($name) {
    if ($name == 'options') {
      return array_merge(provision_sitealias_get_record($this->name), array_filter(drush_get_context('stdin')), array_filter(drush_get_context('options')));
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
    return isset($this->properties[$name]) || property_exists($this, $name);
  }

  /**
   * Remove the value from the properties array if the property does not exist
   * in the class proper.
   */
  function __unset($name) {
    if (isset($this->properties[$name])) {
      unset($this->properties[$name]);
    }
    elseif (property_exists($this, $name)) {
      unset($this->$name);
    }
  }

  /**
   * Implement the __call magic method.
   *
   * This implementation is really simple. It simply return null if the
   * method doesn't exist.
   *
   * This is used so that we can create methods for drush commands, and
   * can fail safely.
   */
  function __call($name, $args) {
    $this->method_invoke($name, $args);

    return false;
  }

  /**
   * Execute a method on the object and all of it's associated
   * services.
   */
  function method_invoke($func, $args = array()) {
    if (method_exists($this, $func)) {
      call_user_func_array(array($this, $func), $args);
    }
    // Services will be invoked regardless of the existence of a 
    // implementation in the environment class.
    $this->services_invoke($func, $args);
  }
  /**
   * Execute the method for the current object type.
   *
   * This function is used to avoid having to conditionally
   * check the context objects type to execute the correct code.
   *
   * This will generate a function call like : $method_$type,
   * ie: $this->init_server().
   *
   * Additionally it will dispatch this function call to
   * all the currently enabled services.
   */
  function type_invoke($name, $args = array()) {
    $this->method_invoke("{$name}_{$this->type}");
  }

  /**
   * Allow a server to plug into a drush command that has been called.
   *
   * This method provides a general case for extending drush commands.
   * This allows the developer to not have to conditionally check the
   * context object type in all his methods, and reduces the need
   * to define drush_hook_$command methods for a lot of cases.
   * 
   * This will generate a function call like : $method_$type_cmd.
   */
  function command_invoke($command, $args = array()) {
    $this->method_invoke("{$command}_{$this->type}_cmd");
  }

  /**
   * Constructor for the environment.
   */
  function __construct($name) {
    $this->name = $name;
  }

  /**
   * Init stub function.
   */
  function init() {
    preg_match("/^provisionEnvironment_(.*)$/", get_class($this), $matches);
    $this->type = $matches[1];
    $this->setProperty('context_type', $this->type);

    // Set up subscriptions for the available services.
    $service_list = drush_command_invoke_all('provision_services');
    foreach ($service_list as $service => $default) {
      $class = "provisionService_{$service}";
      $func = "subcribe_{$this->type}";
      if (method_exists($class, $func)) {
        call_user_func(array($class, $func), $this);
      }
    }
    return true;
  }

  /**
   * Check the $options property for a field, saving to the properties array.
   */
  function setProperty($field, $default = NULL, $array = FALSE) {
    if (isset($this->options[$field])) {
      if ($this->options[$field] === 'null') {
        $this->$field = $default;
      }
      elseif ($array && !is_array($this->options[$field])) {
        $this->$field = explode(',', $this->options[$field]);
      }
      else {
        $this->$field = $this->options[$field];
      }
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

  /**
   * Subscribe a service handler.
   *
   * All future calls to $this->service($service) will be redirected
   * to the context object of #name you specify.
   */
  function service_subscribe($service, $name) {
    $this->service_subs[$service] = $name;
  }

  /**
   * Return a service object for the specific service type.
   *
   * This will return a specifically subscribed service object when one has
   * been registered with service_subscribe, otherwise it will return the value
   * specified by the property specified by $this->parent_key.
   *
   * @param $service
   *   Service type, such as 'http' or 'db'
   * @param $name
   *   Override service owner with a context name as accepted by d().
   *
   * @return
   *   A provisionService object.
   */
  function service($service, $name = null) {
    if (isset($this->service_subs[$service])) {
      return d($this->service_subs[$service])->service($service, ($name) ? $name : $this->name);
    }
    elseif (!is_null($this->parent_key)) {
      return $this->{$this->parent_key}->service($service, ($name) ? $name : $this->name);
    }
    else {
      return new provisionService_null($this->name);
    }
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
    // fetch the merged list of services.
    // These may be on different servers entirely.
    $services = $this->get_services();
    foreach (array_keys($services) as $service) {
      $results[$service] = $this->service($service)->$callback();
    }
    return $results;
  }

  /**
   * Return the merged list of services and the associated servers supplying them.
   *
   * This function will check with the parent_key to retrieve any service subscription
   * it may have, and will add any additional subscriptions.
   *
   * Once the call chain reaches the @server_master object, it will provide the fallbacks
   * if no subscriptions were available.
   */
  function get_services() {
    $services = array();
    if (!is_null($this->parent_key)) {
      $services = $this->{$this->parent_key}->get_services();
    }

    if (sizeof($this->service_subs)) {
      foreach ($this->service_subs as $service => $server) {
        $services[$service] = $server;
      }
    }

    return $services;
  }

  /**
   * Return environment-specific configuration options for help.
   *
   * @return
   *   array('--option' => 'description')
   */
  static function option_documentation() {
    return array();
  }
}

/**
 * Server environment class.
 *
 * This class bootstraps the Service API by generating server
 * objects for each of the available service types.
 */
class provisionEnvironment_server extends provisionEnvironment {
  /**
   * Associative array of services for this server.
   *
   * @see provisionService
   */
  protected $services = array();

  static function option_documentation() {
    $options = array(
      '--remote_host' => 'server: host name; default localhost',
      '--script_user' => 'server: OS user name; default current user',
      '--aegir_root' => 'server: Aegir root; default ' . getenv('HOME'),
      '--master_url' => 'server: Hostmaster URL',
    );
    foreach (drush_command_invoke_all('provision_services') as $service => $default) {
      $reflect = new reflectionClass('provisionService_' . $service);
      $base_dir = dirname($reflect->getFilename());
      $types = array();
      $options['--' . $service . '_service_type'] = 'placeholder';
      foreach (array_keys(drush_scan_directory($base_dir, '%.*_service\.inc%')) as $service_file) {
        if (preg_match('%^' . $base_dir . '/([a-z]+)/(?:\1)_service.inc$%', $service_file, $match)) {
          $types[] = $match[1];
          include_once($service_file);
          $options = array_merge($options, call_user_func(array(sprintf('provisionService_%s_%s', $service, $match[1]), 'option_documentation')));
        }
      }
      $options['--' . $service . '_service_type'] = 'server: ' . implode(', ', $types) . ', or null; default ' . (empty($default) ? 'null' : $default);
    }
    return $options;
  }

  function init() {
    parent::init();

    $this->setProperty('remote_host', 'localhost');
    if ($this->name == '@server_master') {
      $this->setProperty('aegir_root', getenv('HOME'));
      $this->setProperty('script_user', provision_current_user());
    }
    else {
      $this->aegir_root = d('@server_master')->aegir_root;
      $this->script_user = d('@server_master')->script_user;
    }

    $this->setProperty('ip_addresses', array(), true);

    $this->backup_path = $this->aegir_root . '/backups';
    $this->config_path = $this->aegir_root . '/config/' . ltrim($this->name, '@');
    $this->include_path = $this->aegir_root . '/config/includes';

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

    $type_option = "{$service}_service_type";
    
    $type = isset($this->options[$type_option]) ? $this->options[$type_option] : $default;
    if ($service === 'file') {
      // Force provision-save local
      $command = drush_get_command();
      if (preg_match("/^provision-save\b/", $command['command'])) {
        $type = 'local';
      }
    }
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
    $this->services[$service]->setContext(($name) ? $name : $this->name);
    return $this->services[$service];
  }

  /**
   * Retrieve a list of service objects associated with this server.
   */
  function get_services() {
    $services = array();
    foreach ($this->services as $service => $object) {
      $services[$service] = $this->name;
    }
    return $services;
  }


  function verify() {
    $this->type_invoke('verify');
  }

  /**
   * Execute $command on this server, using SSH if necessary.
   *
   * @param $command
   *   Shell command to execute.
   *
   * @return
   *   Same as drush_shell_exec(). Use drush_shell_exec_output() for standard
   *   out and error.
   */
  function shell_exec($command) {
    if (provision_is_local_host($this->remote_host)) {
      return drush_shell_exec(escapeshellcmd($command));
    }
    else {
      return drush_shell_exec('ssh ' . drush_get_option('ssh-options', '-o PasswordAuthentication=no') . ' %s %s', $this->script_user . '@' . $this->remote_host, escapeshellcmd($command));
    }
  }

  /**
   * If necessary, sync files out to a remote server.
   *
   * @param $path
   *   Full path to sync.
   * @param $additional_options
   *   An array of options that overrides whatever was passed in on the command
   *   line (like the 'process' context, but only for the scope of this one
   *   call).
   */
  function sync($path = NULL, $additional_options = array()) {
    if (!provision_is_local_host($this->remote_host)) {
      if (is_null($path)) {
        $path = $this->config_path;
      }

      if (provision_file()->exists($path)->status()) {
        $options = array_merge(array(
          'relative' => TRUE,
          'omit-dir-times' => TRUE,
        ), $additional_options);

        // We need to do this due to how drush creates the rsync command.
        // If the option is present at all , even if false or null, it will
        // add it to the command.
        if (!isset($additional_options['no-delete'])) {
          $options['delete'] = TRUE;
        }

        if (drush_core_call_rsync(escapeshellarg($path), escapeshellarg($this->script_user . '@' . $this->remote_host . ':/'), $options, TRUE, FALSE)) {
          drush_log(dt('@path has been synced to remote server @remote_host.', array('@path' => $path, '@remote_host' => $this->remote_host)));
        }
        else {
          drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('@path could not be synced to remote server @remote_host. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '@remote_host' => $this->remote_host, '%msg' => join("\n", drush_shell_exec_output()))));
        }
      }
      else { // File does not exist, remove it.
        if ($this->shell_exec('rm -rf ' . escapeshellarg($path))) {
          drush_log(dt('@path has been removed from remote server @remote_host.', array('@path' => $path, '@remote_host' => $this->remote_host)));
        }
        else {
          drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('@path could not be removed from remote server @remote_host. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '@remote_host' => $this->remote_host, '%msg' => join("\n", drush_shell_exec_output()))));
        }
      }
    }
  }

  /**
   * If necessary, fetch file from a remote server.
   *
   * @param $path
   *   Full path to fetch.
   */
  function fetch($path) {
    if (!provision_is_local_host($this->remote_host)) {
      if (provision_file()->exists($path)->status()) {
        $options = array(
          'omit-dir-times' => TRUE,
        );
        if (drush_core_call_rsync(escapeshellarg($this->script_user . '@' . $this->remote_host . ':/') . $path, $path, $options, TRUE, FALSE)) {
          drush_log(dt('@path has been fetched from remote server @remote_host.', array(
            '@path' => $path, 
            '@remote_host' => $this->remote_host))
          );
        }
        else {
          drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('@path could not be fetched from remote server @remote_host.' .
            ' Changes might not be available until this has been done. (error: %msg)', array(
              '@path' => $path, 
              '@remote_host' => $this->remote_host, 
              '%msg' => join("\n", drush_shell_exec_output())))
          );
        }
      }
    }
  }
}

/**
 * Class for the platform environment.
 */
class provisionEnvironment_platform extends provisionEnvironment {
  public $parent_key = 'server';

  static function option_documentation() {
    return array(
      '--root' => 'platform: path to a Drupal installation',
      '--server' => 'drush backend server; default @server_master',
      '--web_server' => 'web server hosting the platform; default @server_master',
    );
  }

  function init() {
    $this->parent_key = 'server';

    parent::init();
    $this->setProperty('root');

    $this->setProperty('server', '@server_master');
    $this->is_oid('server');

    $this->setProperty('makefile', '');
  }

  function verify() {
    $this->service('http')->verify();
  }
}

class provisionEnvironment_site extends provisionEnvironment {
  public $parent_key = 'platform';

  static function option_documentation() {
    return array(
      '--platform' => 'site: the platform the site is run on',
      '--db_server' => 'site: the db server the site is run on',
      '--uri' => 'site: example.com URI, no http:// or trailing /',
      '--language' => 'site: site language; default en',
      '--aliases' => 'site: comma-separated URIs',
      '--redirection' => 'site: boolean for whether --aliases should redirect; default false',
      '--client_email' => 'site: email for the site owner',
      '--profile' => 'site: Drupal profile to use; default default',
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

    // set this because this path is accessed a lot in the code, especially in config files.
    $this->site_path = $this->root . '/sites/' . $this->uri;

    $this->setProperty('site_enabled', true);
    $this->setProperty('language', 'en');
    $this->setProperty('client_email');
    $this->setProperty('aliases', array(), TRUE);
    $this->setProperty('redirection', FALSE);

    // TODO :: figure out how to put this into service classes.
    $this->setProperty('ssl_enabled', 0);
    $this->setProperty('ssl_key', null);

    // this can potentially be handled by a Drupal sub class
    $this->setProperty('profile', 'default');
  }
}

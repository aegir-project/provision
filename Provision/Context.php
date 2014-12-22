<?php
/**
 * @file
 * Provides the Provision_Context class.
 */

/**
 * Base context class.
 *
 * Contains magic getter/setter functions
 */
class Provision_Context {
  /**
   * Name for saving aliases and referencing.
   */
  public $name = NULL;

  /**
   * 'server', 'platform', or 'site'.
   */
  public $type = NULL;

  /**
   * Properties that will be persisted by provision-save. Access as object
   * members, $envoronment->property_name. __get() and __set handle this. In
   * init(), set defaults with setProperty().
   */
  protected $properties = array();

  /**
   * Keeps track of properites that are names of Provision_Context objects.
   * Set with is_oid().
   */
  protected $oid_map = array();

  protected $service_subs = array();
  protected $parent_key = NULL;

  /**
   * Retrieve value from $properties array if property does not exist in class
   * proper. Properties that refer to Provision_Context objects will be run
   * through d(), see is_oid().
   *
   * TODO: consider returning a reference to the value, so we can do things like:
   *       `$this->options['option'] = 'value'` and it will correctly set it in the
   *       drush context cache.
   */
  function __get($name) {
    if ($name == 'options') {
      return array_merge(provision_sitealias_get_record($this->name), array_filter(drush_get_context('stdin')), array_filter(drush_get_context('options')), array_filter(drush_get_context('cli')));
    }
    if (array_key_exists($name, $this->properties)) {
      if (isset($this->oid_map[$name]) && !empty($this->properties[$name])) {
        $service = d($this->properties[$name], FALSE, FALSE);
        if (!is_null($service)) {
          return $service;
        }
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
   * This implementation is really simple. It simply return NULL if the
   * method doesn't exist.
   *
   * This is used so that we can create methods for drush commands, and
   * can fail safely.
   */
  function __call($name, $args) {
    return $this->method_invoke($name, $args);
  }

  /**
   * Execute a method on the object and all of it's associated
   * services.
   */
  function method_invoke($func, $args = array(), $services = TRUE) {
    provision::method_invoke($this, $func, $args);
    // Services will be invoked regardless of the existence of a
    // implementation in the context class.
    if ($services) {
      $this->services_invoke($func, $args);
    }
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
   * Constructor for the context.
   */
  function __construct($name) {
    $this->name = $name;
  }

  /**
   * Init stub function.
   */
  function init() {
    preg_match("/^Provision_Context_(.*)$/", get_class($this), $matches);
    $this->type = $matches[1];
    $this->setProperty('context_type', $this->type);

    // Set up the parent of this context object.
    if (!is_null($this->parent_key)) {
      $this->setProperty($this->parent_key);
      $this->is_oid($this->parent_key);
    }

    // $this->server is always @server_master
    $this->server = '@server_master';
    $this->is_oid('server');

    // Set up subscriptions for the available services.
    $service_list = drush_command_invoke_all('provision_services');
    foreach ($service_list as $service => $default) {
      $class = "Provision_Service_{$service}";
      $func = "subscribe_{$this->type}";
      if (method_exists($class, $func)) {
        call_user_func(array($class, $func), $this);
      }
    }
    return true;
  }

  /**
   * Check the $options property for a field, saving to the properties array.
   */
  function setProperty($field, $default = NULL, $array = FALSE, $force = FALSE) {
    // If the options already has the $field...
    if (isset($this->options[$field])) {
      // If the options property is null, or property is being forced, used default value.
      if ($this->options[$field] === 'null' || $force) {
        $this->$field = $default;
      }
      // If options property is supposed to be an array but has comma separated values, explode it.
      elseif ($array && !is_array($this->options[$field])) {
        $this->$field = explode(',', $this->options[$field]);
      }
      // Otherwise use options array value for the $field property.
      else {
        $this->$field = $this->options[$field];
      }
    }
    // If the options doesn't have the field, use default value.
    else {
      $this->$field = $default;
    }
  }

  /**
   * Write out this named context to an alias file.
   */
  function write_alias() {
    $config = new Provision_Config_Drushrc_Alias($this->name, $this->properties);
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
   *   A Provision_Service object.
   */
  function service($service, $name = NULL) {
    if (isset($this->service_subs[$service])) {
      return d($this->service_subs[$service])->service($service, ($name) ? $name : $this->name);
    }
    elseif (!is_null($this->parent_key)) {
      return $this->{$this->parent_key}->service($service, ($name) ? $name : $this->name);
    }
    else {
      return new Provision_Service_null($this->name);
    }
  }

  /**
   * Call method $callback on each of the context's service objects.
   *
   * @param $callback
   *   A Provision_Service method.
   * @return
   *   An array of return values from method implementations.
   */
  function services_invoke($callback, $args = array()) {
    $results = array();
    // fetch the merged list of services.
    // These may be on different servers entirely.
    $services = $this->get_services();
    foreach (array_keys($services) as $service) {
      $results[$service] = provision::method_invoke($this->service($service), $callback, $args);
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
   * Return context-specific configuration options for help.
   *
   * @return
   *   array('option' => 'description')
   */
  static function option_documentation() {
    return array();
  }
}

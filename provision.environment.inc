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
  $object->clean();
  return $object;
}

class provisionEnvironment {
  protected $options = array();
  protected $properties = array();
  protected $map = array();
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
    $this->setProperty('config_path', $this->aegir_root . '/config');
    $this->setProperty('backup_path', $this->aegir_root . '/backup');
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
  }
}



<?php

class provisionService {
  protected $configs = array();
  function verify() {
    return TRUE;
  }

  function templates() {
    $this->configs = array();
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

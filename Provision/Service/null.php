<?php

/**
 * Null service class.
 *
 * Not all services are necessary or turned on.
 * This class ensures that not having a specific service
 * doesnt result in catastrophic failure.
 */
class Provision_Service_null extends Provision_Service {

  function __get($name) {
    return false;
  }

  function __call($name, $args = array()) {
    return false;
  }

  /**
   * Null services do not synch files to the remote server,
   * because they have no associated config files.
   */
  function sync() {
    return null;
  }
}

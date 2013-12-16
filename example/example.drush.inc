<?php

/**
 * @file
 *   An example of the provision service API.
 *
 *  Declares a new service type and a basic implementation of it.
 *  It matches the same service definition in the hosting front end.
 */

/**
 * Implements hook_drush_init().
 */
function example_drush_init() {
  example_provision_register_autoload();
}

/**
 * Register our directory as a place to find provision classes.
 *
 * This is needed so that provision can autoload our classes, which means we
 * don't need to specifically include the files before we use the class, which
 * is quite useful!
 */
function example_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Expose the service type this extension defines to provision.
 *
 * Implementations are then conditionally loaded when the option 
 * "--$service_service_type=$type" is passed to the provision-save
 * command of a server.
 *
 * Implementations are automatically loaded from :
 * provision/$service/$type/$type_service.inc.
 *
 * @return
 *   An array with the service type the key, and the default implementation the value.
 */
function example_provision_services() {
  example_provision_register_autoload();
  return array('example' => NULL);
}

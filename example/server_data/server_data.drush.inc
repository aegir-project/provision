<?php

/**
 * Implements hook_drush_init().
 */
function server_data_drush_init() {
  server_data_provision_register_autoload();
}

/**
 * Register our directory as a place to find provision classes.
 */
function server_data_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Expose the service type this extension defines to provision.
 *
 * @return
 *   An array with the service type the key, and the default implementation the value.
 */
function server_data_provision_services() {
  server_data_provision_register_autoload();
  return array('server_data' => NULL);
}

/**
 * Implements drush_hook_post_COMMAND().
 */
function drush_server_data_post_provision_install() {
  drush_log('server_data received from the frontend (d()->server_data): ' . d()->server_data);
}

function drush_server_data_post_provision_verify() {
  drush_log('server_data received from the frontend (d()->server_data): ' . d()->server_data);
}

<?php

/**
 * Implements hook_drush_init().
 */
function http_drush_init() {
  http_provision_register_autoload();
}

/**
 * Register our directory as a place to find provision classes.
 */
function http_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Implements hook_provision_services().
 */
function http_provision_services() {
  http_provision_register_autoload();
  return array('http' => NULL);
}

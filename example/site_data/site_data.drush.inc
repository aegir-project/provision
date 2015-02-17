<?php

/**
 * @file
 * Example code for the fictional site_data module.
 */

/**
 * Implements hook_drush_init().
 */
function site_data_drush_init() {
  site_data_provision_register_autoload();
}

/**
 * Register our directory as a place to find provision classes.
 */
function site_data_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Expose the service type this extension defines to provision.
 *
 * @return array
 *   An array with the service type as key,
 *   and the default implementation the value.
 */
function site_data_provision_services() {
  site_data_provision_register_autoload();
  return array('site_data' => NULL);
}

/**
 * Implements drush_hook_post_COMMAND().
 */
function drush_site_data_post_provision_install() {
  drush_log('site_data received from the frontend (d()->site_data): ' . d()->site_data);
}

/**
 * Implements drush_hook_post_COMMAND().
 */
function drush_site_data_post_provision_verify() {
  drush_log('site_data received from the frontend (d()->site_data): ' . d()->site_data);
}

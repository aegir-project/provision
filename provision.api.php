<?php

/**
 * @file Provision API
 *
 * @see drush.api.php
 * @see drush_command_invoke_all
 */


/**
 * Advertise what service types are available and their default
 * implementations. Services are class provisionService_{type}_{service} in
 * {type}/{service}/{service}_service.inc files.
 *
 * @return
 *   An associative array of type => default. Default may be NULL.
 *
 * @see provision.service.inc
 */
function drush_hook_provision_services() {
  return array('db' => NULL);
}

/**
 * Append PHP code to Drupal's settings.php file.
 * 
 * To use templating, return an include statement for the template.
 *
 * @param $uri
 *   URI for the site.
 * @param $data
 *   Associative array of data from provisionConfig_drupal_settings::data.
 *
 * @return
 *   Lines to add to the site's settings.php file.
 *
 * @see provisionConfig_drupal_settings
 */
function drush_hook_provision_drupal_config($uri, $data) {
  return '$conf[\'reverse_proxy\'] = TRUE;';
}

/**
 * Append Apache configuration to server configuration.
 * 
 * To use templating, return an include statement for the template.
 *
 * @param $data
 *   Associative array of data from provisionConfig_apache_server::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see provisionConfig_apache_server
 */
function drush_hook_provision_apache_server_config($data) {
}

/**
 * Append Apache configuration to platform configuration.
 * 
 * To use templating, return an include statement for the template.
 *
 * @param $data
 *   Associative array of data from provisionConfig_apache_platform::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see provisionConfig_apache_platform
 */
function drush_hook_provision_apache_dir_config($data) {
}

/**
 * Append Apache configuration to site vhost configuration.
 * 
 * To use templating, return an include statement for the template.
 *
 * @param $uri
 *   URI for the site.
 * @param $data
 *   Associative array of data from provisionConfig_apache_site::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see provisionConfig_apache_site
 */
function drush_hook_provision_apache_vhost_config($uri, $data) {
}

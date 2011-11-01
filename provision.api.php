<?php

/**
 * @file Provision API
 *
 * @see drush.api.php
 * @see drush_command_invoke_all
 */


/**
 * Advertise what service types are available and their default
 * implementations. Services are class Provision_Service_{type}_{service} in
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
 *   Associative array of data from Provision_Config_Drupal_Settings::data.
 *
 * @return
 *   Lines to add to the site's settings.php file.
 *
 * @see Provision_Config_Drupal_Settings
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
 *   Associative array of data from Provision_Config_Apache_Server::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Apache_Server
 */
function drush_hook_provision_apache_server_config($data) {
}

/**
 * Append Apache configuration to platform configuration.
 * 
 * To use templating, return an include statement for the template.
 *
 * @param $data
 *   Associative array of data from ProvisionConfig_Apache_Platform::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see ProvisionConfig_Apache_Platform
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
 *   Associative array of data from Provision_Config_Apache_Site::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Apache_Site
 */
function drush_hook_provision_apache_vhost_config($uri, $data) {
}

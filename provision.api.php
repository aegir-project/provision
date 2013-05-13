<?php
/**
 * @file
 * Provision API
 *
 * @see drush.api.php
 * @see drush_command_invoke_all()
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
 *   Associative array of data from Provision_Config_Apache_Platform::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Apache_Platform
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

/**
 * Append Nginx configuration to server configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * @param $data
 *   Associative array of data from Provision_Config_Nginx_Server::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Nginx_Server
 */
function drush_hook_provision_nginx_server_config($data) {
}

/**
 * Append Nginx configuration to platform configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * @param $data
 *   Associative array of data from Provision_Config_Nginx_Platform::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Nginx_Platform
 */
function drush_hook_provision_nginx_dir_config($data) {
}

/**
 * Append Nginx configuration to site vhost configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * @param $uri
 *   URI for the site.
 * @param $data
 *   Associative array of data from Provision_Config_Nginx_Site::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Nginx_Site
 */
function drush_hook_provision_nginx_vhost_config($uri, $data) {
}

/**
 * Specify a different template for rendering a config file.
 *
 * @param $config
 *   The Provision_config object trying to find its template.
 *
 * @return
 *   A filename of a template to use for rendering.
 *
 * @see hook_provision_config_load_templates_alter()
 */
function hook_provision_config_load_templates($config) {
  if (is_a($config, 'Provision_Config_Drupal_Settings')) {
    $file = dirname(__FILE__) . '/custom-php-settings.tpl.php';
    return $file;
  }
}

/**
 * Alter the templates suggested for rendering a config file.
 *
 * @param $templates
 *   The array of templates suggested by other Drush commands.
 * @param $config
 *   The Provision_config object trying to find its template.
 *
 * @see hook_provision_config_load_templates()
 */
function hook_provision_config_load_templates_alter(&$templates, $config) {
  // Don't let any custom templates be used.
  $templates = array();
}

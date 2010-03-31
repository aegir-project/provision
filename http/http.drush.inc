<?php

include_once(dirname(__FILE__) . '/../provision.service.inc');

function http_drush_init() {
 $command = drush_get_command();
  $command = explode(" ", $command['command']);
  if (preg_match("/^provision-/", $command[0])) {
    provisionService::spawn('http', 'apache'); 
  }
}

class provisionService_http extends provisionService {
  function init() {
    // Set up defines for platform
    $docroot = drush_get_option(array("r", "root"), $_SERVER['PWD']);

    $path = drush_set_default('docroot_path', rtrim(($docroot) ? $docroot : $_SERVER['DOCUMENT_ROOT'], '/'));
    drush_set_default('sites_path', $path . '/sites');
    drush_set_default('publish_path', realpath(drush_get_option('docroot_path')));

    $parts = explode("/", $path);
    array_pop($parts);
    $parent_path = drush_set_default('parent_path', implode("/" , $parts));

    drush_set_default('backup_path', $parent_path . '/backups');
    drush_set_default('config_path', $parent_path . '/config');

    $config_path = drush_get_option('config_path');
    drush_set_default('vhost_path', $config_path . '/vhost.d');
    drush_set_default('platform_conf_path', $config_path . '/platform.d');

    // Commands
    drush_set_default('restart_cmd', _provision_default_restart_cmd());

    // System account
    drush_set_default('web_group', _provision_default_web_group());
    drush_set_default('script_user', get_current_user());

    // Redirection urls
    drush_set_default('master_url', $GLOBALS['base_url']);
    $master_url = drush_get_option('master_url');
    drush_set_default('web_disable_url', $master_url .'/hosting/disabled');
    drush_set_default('web_maintenence_url', $master_url .'/hosting/maintenance');

    drush_set_default('web_ip', '127.0.0.1');
    drush_set_default('web_port', 80);

    drush_set_default('site_port', 80);
  }

  /**
   * Ask the web server to check for and load configuration changes.
   */
  function parse_configs() {
    return TRUE;
  }

  /**
   * Generate a site specific configuration file
   */
  function create_site_config($url) {
    return TRUE;
  }

  /**
   * Remove an existing site configuration file.
   */
  function delete_site_config($url) {
    return TRUE;
  }

  /**
   * Add a new platform specific configuration file.
   */
  function create_platform_config($url) {
    return TRUE;
  }

  /**
   * Remove an existing platform configuration file.
   */
  function delete_platform_config() {
    return TRUE;
  }

  /**
   * Create a new server specific configuration file.
   */
  function create_server_config() {
    return TRUE;

  }

  /**
   * Remove an existing server specific configuration file
   */
  function delete_server_config() {
    return TRUE;
  }
}

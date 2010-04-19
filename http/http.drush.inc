<?php

include_once(dirname(__FILE__) . '/../provision.service.inc');

function http_drush_init($url = null) {
  // todo same as http, consolidate
  $command = drush_get_command();
  $command = explode(" ", $command['command']);
  if (preg_match("/^provision-/", $command[0])) {
    _provision_context_init($url);

    if (PROVISION_CONTEXT_SERVER) {
      if (!is_null(drush_get_option('init_http'))) {
        // Bootstrap for alias creation.
        include_once(drush_get_option('init_http') . '/'. drush_get_option('init_http') .'_service.inc');
        $class = 'provisionService_http_' . drush_get_option('init_http');
        provision_service('http', new $class());
      }
    }
    else {
      // todo should be constructed based on drush aliases
      include_once('apache/apache_service.inc');
      provision_service('http', new provisionService_http_apache());
    }
  }
}

function http_drush_exit() {
  if (PROVISION_CONTEXT_SERVER) {
    provision_service('http')->write_alias();
  }
}

class provisionService_http extends provisionService {
  /**
   * Set defaults
   */
  function __construct() {
    // Set up defines for platform
    $docroot = drush_get_option(array("r", "root"), $_SERVER['PWD']);

    $path = drush_set_default('docroot_path', rtrim(($docroot) ? $docroot : $_SERVER['DOCUMENT_ROOT'], '/'));
    drush_set_default('sites_path', $path . '/sites');
    drush_set_default('publish_path', realpath(drush_get_option('docroot_path')));

    $parts = explode("/", $path);
    array_pop($parts);
    drush_set_default('parent_path', implode("/" , $parts));

    $parent_path = drush_get_option('parent_path');
    drush_set_default('backup_path', $parent_path . '/backups');
    drush_set_default('config_path', $parent_path . '/config');

    $config_path = drush_get_option('config_path');
    drush_set_default('vhost_path', $config_path . '/vhost.d');
    drush_set_default('platform_conf_path', $config_path . '/platform.d');

    // Commands
    drush_set_default('restart_cmd', _provision_default_restart_cmd());

    // System account
    drush_set_default('web_group', _provision_default_web_group());
    $pw = posix_getpwuid(posix_geteuid());
    drush_set_default('script_user', $pw['name']);

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

  /**
   * Write out server's drushrc alias file.
   */
  function write_alias() {
    $config = new provisionConfig_drushrc_alias(drush_get_option('web_host'), array('web_id', 'web_host', 'web_ports', 'web_group', 'restart_cmd'));
    $config->write();
  }
}

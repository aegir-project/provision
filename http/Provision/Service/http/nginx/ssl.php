<?php

/**
 * Nginx SSL service class.
 *
 * This class doesn't extend the nginx service itself, so there may
 * be some duplication of code between them. The majority of the 
 * functionality is however implemented in the Provision_Service_http_public
 * class, which we do extend.
 */
class Provision_Service_http_nginx_ssl extends Provision_Service_http_ssl {
  // We share the application name with nginx.
  protected $application_name = 'nginx';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    // The nginx service defines it's restart command as a static
    // method so that we can make use of it here.
    return Provision_Service_http_nginx::nginx_restart_cmd();
  } 

  public $ssl_enabled = TRUE;

  /**
   * Initialize the configuration files.
   *
   * These config classes are a mix of the SSL and Non-SSL nginx
   * classes. In some cases they extend the Nginx classes too.
   */
  function init_server() {
    parent::init_server();
    // Replace the server config with our own. See the class for more info.
    $this->configs['server'][] = 'Provision_Config_Nginx_Ssl_Server';
    $this->configs['site'][] = 'Provision_Config_Nginx_Ssl_Site';
  }

  function verify_server_cmd() {
     provision_file()->copy(str_replace('//','/',dirname(__FILE__).'/') . '../nginx_advanced_include.conf', $this->server->include_path . '/nginx_advanced_include.conf');
     $this->sync($this->server->include_path . '/nginx_advanced_include.conf');
     provision_file()->copy(str_replace('//','/',dirname(__FILE__).'/') . '../nginx_simple_include.conf', $this->server->include_path . '/nginx_simple_include.conf');
     $this->sync($this->server->include_path . '/nginx_simple_include.conf');
     provision_file()->copy(str_replace('//','/',dirname(__FILE__).'/') . '../fastcgi_params.conf', $this->server->include_path . '/fastcgi_params.conf');
     $this->sync($this->server->include_path . '/fastcgi_params.conf');
     provision_file()->copy(str_replace('//','/',dirname(__FILE__).'/') . '../fastcgi_ssl_params.conf', $this->server->include_path . '/fastcgi_ssl_params.conf');
     $this->sync($this->server->include_path . '/fastcgi_ssl_params.conf');
     provision_file()->copy(str_replace('//','/',dirname(__FILE__).'/') . '../upload_progress_test.conf', $this->server->include_path . '/upload_progress_test.conf');
     $this->sync($this->server->include_path . '/upload_progress_test.conf');
    // Call the parent at the end. it will restart the server when it finishes.
    parent::verify_server_cmd();
  }

  /**
   * Restart/reload nginx to pick up the new config files.
   */ 
  function parse_configs() {
    return $this->restart();
  }
}

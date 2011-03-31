<?php

// $Id$

/**
 * @file
 *    The Nginx +SSL service implementation.
 */

// Include the nginx service implementation. 
// Even though we don't extend it, we do make use of it's config classes.
include_once(dirname(__FILE__) . '/../nginx/nginx_service.inc');


/**
 * Nginx SSL service class.
 *
 * This class doesn't extend the nginx service itself, so there may
 * be some duplication of code between them. The majority of the 
 * functionality is however implemented in the provisionService_http_public
 * class, which we do extend.
 */
class provisionService_http_nginx_ssl extends provisionService_http_ssl {
  // We share the application name with nginx.
  protected $application_name = 'nginx';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    // The nginx service defines it's restart command as a static
    // method so that we can make use of it here.
    return provisionService_http_nginx::nginx_restart_cmd();
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
    $this->configs['server'][] = 'provisionConfig_nginx_ssl_server';
    $this->configs['site'][] = 'provisionConfig_nginx_ssl_site';
  }

  function verify_server_cmd() {
     provision_file()->copy(dirname(__FILE__) . '/nginx_advanced_include.conf', $this->server->include_path . '/nginx_advanced_include.conf');
     $this->sync($this->server->include_path . '/nginx_advanced_include.conf');
     provision_file()->copy(dirname(__FILE__) . '/nginx_simple_include.conf', $this->server->include_path . '/nginx_simple_include.conf');
     $this->sync($this->server->include_path . '/nginx_simple_include.conf');
     provision_file()->copy(dirname(__FILE__) . '/fastcgi_params.conf', $this->server->include_path . '/fastcgi_params.conf');
     $this->sync($this->server->include_path . '/fastcgi_params.conf');
     provision_file()->copy(dirname(__FILE__) . '/fastcgi_ssl_params.conf', $this->server->include_path . '/fastcgi_ssl_params.conf');
     $this->sync($this->server->include_path . '/fastcgi_ssl_params.conf');
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

/**
 * Server config file for Nginx + SSL.
 *
 * This configuration file replaces the Nginx server configuration file, but
 * inside the template, the original file is once again included.
 *
 * This config is primarily reponsible for enabling the SSL relation settings,
 * so that individual sites can just enable them.
 */
class provisionConfig_nginx_ssl_server extends provisionConfig_http_ssl_server {
  // We use the same extra_config as the nginx_server config class.
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_server_config', $this->data));
  }
}

/**
 * Virtual host config file for Nginx + SSL.
 *
 * This file is created in addition to the existing virtual host, 
 * and includes some extra directives.
 */
class provisionConfig_nginx_ssl_site extends provisionConfig_http_ssl_site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_vhost_config', $this->uri, $this->data));
  }
}

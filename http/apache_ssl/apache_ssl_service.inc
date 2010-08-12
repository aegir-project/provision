<?php

// $Id$

/**
 * @file
 *    The Apache +SSL service implementation.
 */

// Include the apache service implementation. 
// Even though we don't extend it, we do make use of it's config classes.
include_once(dirname(__FILE__) . '/../apache/apache_service.inc');


/**
 * Apache SSL service class.
 *
 * This class doesn't extend the apache service itself, so there may
 * be some duplication of code between them. The majority of the 
 * functionality is however implemented in the provisionService_http_public
 * class, which we do extend.
 */
class provisionService_http_apache_ssl extends provisionService_http_ssl {
  // We share the application name with apache.
  protected $application_name = 'apache';
  protected $has_restart_cmd = TRUE;
  
  function default_restart_cmd() {
    // The apache service defines it's restart command as a static
    // method so that we can make use of it here.
    return provisionService_http_apache::apache_restart_cmd();
  } 

  public $ssl_enabled = TRUE;

  function cloaked_db_creds() {
    return TRUE;
  }

  /**
   * Initialize the configuration files.
   *
   * These config classes are a mix of the SSL and Non-SSL apache
   * classes. In some cases they extend the Apache classes too.
   */
  function init_server() {
    parent::init_server();

    // Replace the server config with our own. See the class for more info.
    $this->configs['server'][] = 'provisionConfig_apache_ssl_server';

    // Just re-use the standard platform config.
    $this->configs['platform'][] = 'provisionConfig_apache_platform';

    $this->configs['site'][] = 'provisionConfig_apache_ssl_site';
  }

  /**
   * Restart apache to pick up the new config files.
   */ 
  function parse_configs() {
    return $this->restart();
  }
}

/**
 * Server config file for Apache + SSL.
 *
 * This configuration file replaces the Apache server configuration file, but
 * inside the template, the original file is once again included.
 *
 * This config is primarily reponsible for enabling the SSL relation settings,
 * so that individual sites can just enable them.
 */
class provisionConfig_apache_ssl_server extends provisionConfig_http_ssl_server {
  // We use the same extra_config as the apache_server config class.
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

/**
 * Virtual host config file for Apache + SSL.
 *
 * This file is created in addition to the existing virtual host, 
 * and includes some extra directives.
 */
class provisionConfig_apache_ssl_site extends provisionConfig_http_ssl_site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->uri, $this->data));
  }
}

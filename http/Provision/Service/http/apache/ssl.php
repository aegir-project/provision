<?php

/**
 * Apache SSL service class.
 *
 * This class doesn't extend the apache service itself, so there may
 * be some duplication of code between them. The majority of the 
 * functionality is however implemented in the Provision_Service_http_public
 * class, which we do extend.
 */
class Provision_Service_http_apache_ssl extends Provision_Service_http_ssl {
  // We share the application name with apache.
  protected $application_name = 'apache';
  protected $has_restart_cmd = TRUE;
  
  function default_restart_cmd() {
    // The apache service defines it's restart command as a static
    // method so that we can make use of it here.
    return Provision_Service_http_apache::apache_restart_cmd();
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
    $this->configs['server'][] = 'Provision_Config_Apache_Ssl_Server';

    // Just re-use the standard platform config.
    $this->configs['platform'][] = 'Provision_Config_Apache_Platform';

    $this->configs['site'][] = 'Provision_Config_Apache_Ssl_Site';
  }

  /**
   * Restart apache to pick up the new config files.
   */ 
  function parse_configs() {
    return $this->restart();
  }
}

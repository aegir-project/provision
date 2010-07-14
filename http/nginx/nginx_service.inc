<?php
/**
 * @file - nginx service for aegir.
 */

class provisionService_http_nginx extends provisionService_http_public {
  protected $application_name = 'nginx';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    return provisionService_http_nginx::nginx_restart_cmd();
  }


  function init() {
    parent::init();
    $this->configs['server'][] = 'provisionConfig_nginx_server';
    $this->configs['site'][] = 'provisionConfig_nginx_site';
  }

  /**
   * Guess at the likely value of the http_restart_cmd.
   *
   * This method is a static so that it can be re-used by the nginx_ssl
   * service, even though it does not inherit this class.
   */
  public static function nginx_restart_cmd() {
    return 'sudo /etc/init.d/nginx restart';
  }

  /**
   * Restart nginx to pick up the new config files.
   */ 
  function parse_configs() {
    return $this->restart();
  }
}

/**
 * Apache server level configuration file class
 */
class provisionConfig_nginx_server extends provisionConfig_http_server {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_server_config', $this->data));
  }
}

/** 
 * Apache site level config class. Virtual host.
 */
class provisionConfig_nginx_site extends provisionConfig_http_site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_vhost_config', $this->uri, $this->data));
  }
}





<?php

include_once(dirname(__FILE__) . '/../provision.service.inc');


function http_provision_services() {
  return array('http' => NULL);
}

class provisionService_http extends provisionService {
  public $service = 'http';

  protected $has_port = TRUE;

  function init() {
    parent::init();
    // System account
    if ($this->server->name == '@server_master') {
      $this->server->setProperty('web_group', _provision_default_web_group());
    }
    else {
      $this->server->web_group = d('@server_master')->web_group;
    }


    // Redirection urls
    $this->server->setProperty('web_disable_url', $this->server->master_url .'/hosting/disabled');
    $this->server->setProperty('web_maintenence_url', $this->server->master_url .'/hosting/maintenance');
  }

  function default_port() {
    return 80;
  }

  static function option_documentation() {
    return array(
      '--web_group' => 'server with http: OS group for permissions; working default will be attepted',
      '--web_disable_url' => 'server with http: URL disabled sites are redirected to; default {master_url}/hosting/disabled',
      '--web_maintenence_url' => 'server with http: URL maintenance sites are redirected to; default {master_url}/hosting/maintenance',
    );
  }


  function verify() {
    switch (d()->type) {
      case 'server':
        $this->create_server_config();
        break;

      case 'platform':
        $this->create_platform_config();
        break;

      case 'site':
        $this->create_site_config();
        break;
    }

    $this->parse_configs();
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
  function create_site_config() {
    return TRUE;
  }

  /**
   * Remove an existing site configuration file.
   */
  function delete_site_config() {
    return TRUE;
  }

  /**
   * Add a new platform specific configuration file.
   */
  function create_platform_config() {
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
   * Support the ability to cloak the database credentials using environment variables.
   */
  function cloaked_db_creds() {
    return FALSE;
  }

  /**
   * Return a list of servers that will need database access.
   */
  function grant_server_list() {
    return array(
      $this->server,
      d($this->context)->platform->server,
    );
  }
}

<?php

include_once(dirname(__FILE__) . '/../provision.service.inc');


function http_provision_services() {
  return array('http' => 'apache');
}


class provisionService_http extends provisionService {
  function init() {
    parent::init();
    // System account
    $this->setProperty('web_group', _provision_default_web_group());

    // Redirection urls
    $this->setProperty('web_disable_url', $this->master_url .'/hosting/disabled');
    $this->setProperty('web_maintenence_url', $this->master_url .'/hosting/maintenance');

    $this->setProperty('web_ip', '127.0.0.1');
    $this->setProperty('web_port', 80);
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

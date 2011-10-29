<?php

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class Provision_Service_dns_bind extends Provision_Service_dns {
  protected $application_name = 'bind';

  protected $has_restart_cmd = TRUE;
  private $zone_cache = array();
  
  static function bind_default_restart_cmd() {
    return "rndc reload";
  }

  function default_restart_cmd() {
    return Provision_Service_dns_bind::bind_default_restart_cmd();
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'Provision_Config_Bind_Server';
    $this->configs['zone'][] = 'Provision_Config_Bind_Zone';
  }

  function parse_configs() {
    $status = $this->restart();
    return $status && parent::parse_configs();
  }
}

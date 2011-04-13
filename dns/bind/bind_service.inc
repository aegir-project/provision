<?php

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class provisionService_dns_bind extends provisionService_dns {
  protected $application_name = 'bind';

  protected $has_restart_cmd = TRUE;
  private $zone_cache = array();
  
  static function bind_default_restart_cmd() {
    return "rndc reload";
  }

  function default_restart_cmd() {
    return provisionService_dns_bind::bind_default_restart_cmd();
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'provisionConfig_bind_server';
    $this->configs['zone'][] = 'provisionConfig_bind_zone';
  }

  function parse_configs() {
    $status = $this->restart();
    return $status && parent::parse_configs();
  }
}

class provisionConfig_bind_zone extends provisionConfig_dns_zone {

  /**
   * this renders the slave servers names (as their alias is stored)
   */
  function process() {
    parent::process();
    $slaves = array();
    if (!is_array($this->server->slave_servers)) {
      $this->server->slave_servers = array($this->server->slave_servers);
    }
    foreach ($this->server->slave_servers as $slave) {
      $slaves[] = d($slave)->remote_host;
    }
    $this->data['server']->slave_servers_names = $slaves;
  }
}

class provisionConfig_bind_server extends provisionConfig_dns_server {

  /**
   * pre-render the slave servers IP addresses
   *
   * This is done so we can configure the allow-transfer ACL.
   */
  function process() {
    parent::process();
    $slaves = array();
    if (!is_array($this->server->slave_servers)) {
      $this->server->slave_servers = array($this->server->slave_servers);
    }
    foreach ($this->server->slave_servers as $slave) {
      $slaves = array_merge($slaves, d($slave)->ip_addresses);
    }
    $this->data['server']->slave_servers_ips = $slaves;
  }
}

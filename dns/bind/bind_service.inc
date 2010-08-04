<?php

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class provisionService_dns_bind extends provisionService_dns_complex {
  protected $application_name = 'bind';

  protected $has_restart_command = TRUE;
  private $zone_cache = array();
  
  static function bind_default_restart_cmd() {
    return "/etc/init.d/named restart";
  }

  function default_restart_cmd() {
    return provisionService_dns_bind::bind_default_restart_cmd();
  }

  function init() {
    parent::init();
    $this->configs['server'][] = 'provisionConfig_bind_server';
    $this->configs['zone'][] = 'provisionConfig_bind_zone';
  }

  function parse_configs() {
    $this->restart();
  }
}

class provisionConfig_bind_zone extends provisionConfig_dns_zone {
}

class provisionConfig_bind_server extends provisionConfig_dns_server {
}

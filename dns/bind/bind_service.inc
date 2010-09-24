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
    $this->restart();
  }
}

class provisionService_dns_bind_slave extends provisionService_dns {
  protected $application_name = 'bind';

  protected $has_restart_cmd = TRUE;
  
  function default_restart_cmd() {
    return provisionService_dns_bind::bind_default_restart_cmd();
  }

  function init_server() {
    parent::init_server();
    $this->server->setProperty('dns_master_server', null);
    $this->configs['slave'][] = 'provisionConfig_bind_slave';
  }

  function parse_configs() {
    $this->restart();
  }

}

class provisionConfig_bind_zone extends provisionConfig_dns_zone {
}

class provisionConfig_bind_server extends provisionConfig_dns_server {
}

class provisionConfig_bind_slave extends provisionConfig_dns_server {
}

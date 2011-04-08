<?php

class provisionService_dns_dnsmasq extends provisionService_dns {
  protected $application_name = 'dnsmasq';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    return 'sudo /etc/init.d/dnsmasq restart';
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'provisionConfig_dnsmasq_server';
    $this->configs['zone'][] = 'provisionConfig_dnsmasq_zone';
    $this->configs['host'][] = 'provisionConfig_dnsmasq_host';
  }

  function parse_configs() {
    $this->restart();
  }

  function create_host($host = NULL) {
    parent::create_host($host);
    $this->create_config('host');
  }
}

class provisionConfig_dnsmasq_server extends provisionConfig_dns_server {

}

class provisionConfig_dnsmasq_zone extends provisionConfig_dns_zone {

}

class provisionConfig_dnsmasq_host extends provisionConfig_dns_host {

}

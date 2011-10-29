<?php

class Provision_Service_dns_dnsmasq extends Provision_Service_dns {
  protected $application_name = 'dnsmasq';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    return 'sudo /etc/init.d/dnsmasq restart';
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'Provision_Config_Dnsmasq_Server';
    $this->configs['zone'][] = 'Provision_Config_Dnsmasq_Zone';
    $this->configs['host'][] = 'Provision_Config_Dnsmasq_Host';
  }

  function parse_configs() {
    $this->restart();
  }

  function create_host($host = NULL) {
    parent::create_host($host);
    $this->create_config('host');
  }
}

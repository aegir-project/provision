<?php

class Provision_Config_Bind_slave extends provisionConfig_dns_server {
  public $template = 'slave.tpl.php';

  function process() {
    parent::process();
    if ($this->context->type == 'server') {
     $ips = $this->context->ip_addresses;
    }
    else {
     $ips = $this->context->server->ip_addresses;
    }
    $this->data['master_ip_list'] = implode(';', $ips);
  }
}

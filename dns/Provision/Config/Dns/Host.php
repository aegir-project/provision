<?php

class Provision_Config_Dns_Host extends Provision_Config_Dns {
  public $template = 'host.tpl.php';
  public $description = 'Host-wide DNS configuration';

  function filename() {
    return "{$this->data['server']->dns_hostd_path}/{$this->uri}.hosts";
  }
}

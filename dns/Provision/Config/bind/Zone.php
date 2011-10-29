<?php

class Provision_Config_Bind_Zone extends Provision_Config_Dns_Zone {

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

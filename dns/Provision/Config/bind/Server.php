<?php

class Provision_Config_Bind_Server extends Provision_Config_Dns_Server {

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

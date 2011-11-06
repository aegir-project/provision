<?php

// The data store for the server configuration
// contains a list of zones we manage.
class Provision_Config_Dns_Server_Store extends Provision_Config_Data_Store {
  function filename() {
    return $this->data['server']->dns_data_path . '/zones.master.inc';
  }
}

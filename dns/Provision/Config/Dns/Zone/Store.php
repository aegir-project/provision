<?php

class Provision_Config_Dns_Zone_Store extends Provision_Config_Data_Store {
  function filename() {
    return "{$this->data['server']->dns_data_path}/{$this->data['name']}.zone.inc";
  }
}

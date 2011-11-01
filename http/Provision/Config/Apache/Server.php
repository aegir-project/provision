<?php

/**
 * Apache server level configuration file class
 */
class Provision_Config_Apache_Server extends Provision_Config_Http_Server {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

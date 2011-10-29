<?php

/** 
 * Nginx site level config class. Virtual host.
 */
class Provision_Config_Nginx_Site extends Provision_Config_Http_Site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_vhost_config', $this->uri, $this->data));
  }
}

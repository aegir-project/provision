<?php

/**
 * Virtual host config file for Apache + SSL.
 *
 * This file is created in addition to the existing virtual host, 
 * and includes some extra directives.
 */
class Provision_Config_Apache_Ssl_Site extends Provision_Config_Http_Ssl_Site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->uri, $this->data));
  }
}

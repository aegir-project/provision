<?php

/**
 * Server config file for Apache + SSL.
 *
 * This configuration file replaces the Apache server configuration file, but
 * inside the template, the original file is once again included.
 *
 * This config is primarily reponsible for enabling the SSL relation settings,
 * so that individual sites can just enable them.
 */
class Provision_Config_Apache_Ssl_Server extends Provision_Config_Http_Ssl_Server {
  // We use the same extra_config as the apache_server config class.
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

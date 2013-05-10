<?php

/**
 * Nginx server level configuration file class.
 */
class Provision_Config_Nginx_Server extends Provision_Config_Http_Server {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_server_config', $this->data));
  }
}

/**
 * Nginx specific config includes.
 */
class Provision_Config_Nginx_Includes extends Provision_Config_Http_Server {
  public $template = 'vhost_include.tpl.php';
  public $description = 'Nginx web server configuration include file';

  function filename() {
    if (isset($this->data['application_name'])) {
      $file = $this->data['application_name'] . '_vhost_common.conf';
      return $this->data['server']->include_path . '/' . $file
    }
    else {
      return FALSE;
    }
  }
}

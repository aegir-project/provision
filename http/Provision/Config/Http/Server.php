<?php

/**
 * Base configuration class for server level http configs.
 *
 * This class uses the service defined application_name, to generate
 * a top level $app_name.conf for the service.
 *
 * Each server has it's own configuration, and this class will
 * automatically generate a symlink to the correct file for each
 * server.
 */
class Provision_Config_Http_Server extends Provision_Config_Http {
  public $template = 'server.tpl.php';
  public $description = 'web server configuration file';

  function write() {
    parent::write();
  }

  function filename() {
    if (isset($this->data['application_name'])) {
      $file = $this->data['application_name'] . '.conf';
      return $this->data['server']->config_path . '/' . $file;
    }
    else {
      return FALSE;
    }
  }


}

<?php
/**
 * @file Server.php
 *
 *       Apache Configuration for Server Context.
 * @see \Provision_Config_Apache_Server
 * @see \Provision_Config_Http_Server
 * @see \Provision_Config_Http_Server
 */

namespace Aegir\Provision\Service\Http\Apache\Configuration;

use Aegir\Provision\Configuration;

class PlatformConfiguration extends Configuration {
  
  const SERVICE_TYPE = 'apache';
  
  public $template = 'server.tpl.php';
  public $description = 'web server configuration file';
  
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
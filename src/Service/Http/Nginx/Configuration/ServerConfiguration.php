<?php
/**
 * @file ServerConfiguration.php
 *
 * NGINX Configuration for Server Context.
 */

namespace Aegir\Provision\Service\Http\Nginx\Configuration;

use Aegir\Provision\Configuration;

class ServerConfiguration extends Configuration {
  
  public $template = 'templates/server.tpl.php';
  public $description = 'web server configuration file';
  
  function filename() {
    if ($this->service->getType()) {
      $file = $this->service->getType() . '.conf';
      return $this->service->provider->getProvision()->getConfig()->get('config_path') . '/' . $this->service->provider->name . '/' . $file;
    }
    else {
      return FALSE;
    }
  }
}
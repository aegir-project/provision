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
  
  public $template = 'platform.tpl.php';
  public $description = 'platform configuration file';
  
  function filename() {
      $path = $this->context->application->getConfig()->get('config_path') . '/' . $this->service->getType() . '/platform.d/';
    
      return $path . $this->context->name . '.conf';
  }
    
    function process()
    {
        $this->data['http_port']['root'] = 'yeahhh';
        parent::process();
    }
}
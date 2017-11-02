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
      return $this->service->properties['http_platformd_path'] . '/' . ltrim($this->context->name, '@') . '.conf';
  }
}
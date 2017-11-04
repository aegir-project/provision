<?php
/**
 * @file Server.php
 *
 *       Apache Configuration for Server Context.
 *
 *       This class represents the file at /var/aegir/config/apache.conf.
 *
 *
 * @see \Provision_Config_Apache_Server
 * @see \Provision_Config_Http_Server
 * @see \Provision_Config_Http_Server
 */

namespace Aegir\Provision\Service\Http\Apache\Configuration;

use Aegir\Provision\Configuration;

class ServerConfiguration extends Configuration {
  
  const SERVICE_TYPE = 'apache';
  
  public $template = 'server.tpl.php';
  public $description = 'web server configuration file';
  
  function filename() {
    if ($this->service->getType()) {
      $file = $this->service->getType() . '.conf';
      return $this->context->application->getConfig()->get('config_path') . '/' . $this->context->name . '/' . $file;
    }
    else {
      return FALSE;
    }
  }
  function process()
  {
      parent::process();
      $app_dir = $this->context->application->getConfig()->get('config_path') . '/' . $this->service->getType();
      $this->data['http_port'] = $this->service->properties['http_port'];
      $this->data['include_statement'] = '# INCLUDE STATEMENT';
      $this->data['http_pred_path'] = "{$app_dir}/pre.d";
      $this->data['http_postd_path'] = "{$app_dir}/post.d";
      $this->data['http_platformd_path'] = "{$app_dir}/platform.d";
      $this->data['http_vhostd_path'] = "{$app_dir}/vhost.d";
      $this->data['extra_config'] = "";
  }
}
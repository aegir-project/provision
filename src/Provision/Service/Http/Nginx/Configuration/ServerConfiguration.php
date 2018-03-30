<?php
/**
 * @file ServerConfiguration.php
 *
 * NGINX Configuration for Server Context.
 */

namespace Aegir\Provision\Service\Http\Nginx\Configuration;

use Aegir\Provision\ConfigFile;

class ServerConfiguration extends ConfigFile {
  
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

    function process() {
        parent::process();

        // Run verify to load in nginx properties.
        $this->service->verify();

        $app_dir = $this->getContext()->server_config_path . DIRECTORY_SEPARATOR . $this->service->getType();
        $this->data['http_port'] = $this->service->properties['http_port'];
        $this->data['include_statement'] = '# INCLUDE STATEMENT';
        $this->data['http_pred_path'] = "{$app_dir}/pre.d";
        $this->data['http_postd_path'] = "{$app_dir}/post.d";
        $this->data['http_platformd_path'] = "{$app_dir}/platform.d";
        $this->data['http_vhostd_path'] = "{$app_dir}/vhost.d";
        $this->data['extra_config'] = "";

        $this->fs->mkdir($this->data['http_pred_path']);
        $this->fs->mkdir($this->data['http_postd_path']);
        $this->fs->mkdir($this->data['http_platformd_path']);
        $this->fs->mkdir($this->data['http_vhostd_path']);

        $this->data['script_user'] = $this->service->provider->getProperty('script_user');
        $this->data['aegir_root'] = $this->service->provider->getProperty('aegir_root');
    }
}
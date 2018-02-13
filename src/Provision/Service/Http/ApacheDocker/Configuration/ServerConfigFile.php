<?php
namespace Aegir\Provision\Service\Http\ApacheDocker\Configuration;

use Aegir\Provision\Service\Http\Apache\Configuration\ServerConfigFile as BaseServerConfiguration;

class ServerConfigFile extends BaseServerConfiguration {
  
    public $template = '../../Apache/Configuration/server.tpl.php';
    
    function process()
    {
        parent::process();
        
        # Home directory inside the container is not dynamic.
        $app_dir = '/var/aegir/config/'.$this->context->name.'/'
            .$this->service->getType();
        $this->data['http_port'] = $this->service->properties['http_port'];
        $this->data['include_statement'] = '# INCLUDE STATEMENT';
        $this->data['http_pred_path'] = "{$app_dir}/pre.d";
        $this->data['http_postd_path'] = "{$app_dir}/post.d";
        $this->data['http_platformd_path'] = "{$app_dir}/platform.d";
        $this->data['http_vhostd_path'] = "{$app_dir}/vhost.d";
        $this->data['extra_config'] = "";
    }
}
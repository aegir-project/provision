<?php
/**
 * @file ServerConfiguration.php
 *
 * NGINX Configuration for Server Context.
 */

namespace Aegir\Provision\Service\Http\Nginx\Configuration;

use Aegir\Provision\Configuration;

class SiteCommonConfiguration extends ServerConfiguration {

    public $template = 'templates/vhost_include.tpl.php';

    public $description = 'Common site configuration';


    function write() {
        parent::write();
//
//        if (isset($this->data['application_name'])) {
//            $file = $this->data['application_name'] . '_vhost_common.conf';
//            $legacy_simple_file = $this->data['application_name'] . '_simple_include.conf';
//            $legacy_advanced_file = $this->data['application_name'] . '_advanced_include.conf';
//            // We link both legacy files on the remote server to the right version.
//            $cmda = sprintf('ln -sf %s %s',
//                escapeshellarg($this->data['server']->include_path . '/' . $file),
//                escapeshellarg($this->data['server']->include_path . '/' . $legacy_simple_file)
//            );
//            $cmdb = sprintf('ln -sf %s %s',
//                escapeshellarg($this->data['server']->include_path . '/' . $file),
//                escapeshellarg($this->data['server']->include_path . '/' . $legacy_advanced_file)
//            );
//            if ($this->data['server']->shell_exec($cmda)) {
//                drush_log(dt("Created legacy_simple_file symlink for %file on %server", array(
//                    '%file' => $file,
//                    '%server' => $this->data['server']->remote_host,
//                )));
//            };
//            if ($this->data['server']->shell_exec($cmdb)) {
//                drush_log(dt("Created legacy_advanced_file symlink for %file on %server", array(
//                    '%file' => $file,
//                    '%server' => $this->data['server']->remote_host,
//                )));
//            };
//        }
    }

    function filename() {
        return $this->service->provider->server_config_path . '/nginx_vhost_common.conf';
    }
}
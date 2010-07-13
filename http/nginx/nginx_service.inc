<?php
// $Id$
/**
 * @file - nginx service for aegir.
 */

class provisionService_http_nginx extends provisionService_http {

  function init() {
    $this->server->nginx_conf_path = $this->server->config_path . '/nginx/nginx.d';
    $this->server->nginx_site_conf_path = $this->server->config_path . '/nginx/vhost.d';
  }

  function config_data() {
     return array(
      'server' => $this->server,
      'nginx_site_conf_path' => $this->server->nginx_site_conf_path,
      'nginx_conf_path' => $this->server->nginx_conf_path
    );
  }

  function create_site_config() {
    if (d($this->context)->site_enabled) {
      $config = new provisionConfig_nginx_site($this->context, $this->config_data() + drush_get_context('site'));
    }
    else {
      $config = new provisionConfig_nginx_site_disabled($this->context, $this->config_data() + drush_get_context('site'));
    }

    $config->write();
  }

  function delete_site_config() {
    $config = new provisionConfig_nginx_site($this->context, $this->config_data());
    $config->unlink();
  }

  function create_server_config() {
    provision_file()->create_dir($this->server->nginx_site_conf_path, dt("Virtual host configuration"), 0700);
    provision_file()->create_dir($this->server->nginx_conf_path , dt("Nginx configuration"), 0700);
    $this->sync($this->server->nginx_conf_path);

    $config = new provisionConfig_nginx_server($this->context, $this->config_data());
    $config->write();


    // Create a symlink to the site specific nginx config in the top level config directory of each server.
    // This way you can still include the same file across all servers.

    $this->server->shell_exec(sprintf('ln -sf %s %s', escapeshellarg($this->server->config_path . '/nginx.conf'), escapeshellarg($this->server->aegir_root . '/config/nginx.conf')));
  }

  function delete_server_config() {
    $config = new provisionConfig_nginx_server($this->context, $this->config_data());
    $config->unlink();
  }

  function parse_configs() {
    // This is required to be configurable, due to the fact that different
    // hosts might need to do this differently.
    if ($this->server->shell_exec($this->server->restart_cmd)) {
      drush_log(dt('Nginx on %server has been restarted', array('%server' => $this->server->remote_host)));
    }
    else {
      drush_log(dt('Web server %server could not be restarted. Changes might not be available until this has been done. (error: %msg)', array('%server' => $this->server->remote_host, '%msg' => join("\n", drush_shell_exec_output()))), 'warning');
    }
  }
}



class provisionConfig_nginx extends provisionConfig {
  function write() {
    parent::write();
    $this->data['server']->sync($this->filename());
  }

  function unlink() {
    parent::unlink();
    $this->data['server']->sync($this->filename());
  }
}

/**
 * Nginx server level configuration file class
 */
class provisionConfig_nginx_server extends provisionConfig_nginx {
  public $template = 'server.tpl.php';
  public $description = 'nginx server configuration file';

  function filename() {
    dlm($this->data['server']);
    return $this->data['server']->config_path . '/nginx.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_server_config', $this->data));
  }
}


/** 
 * Nginx site level config class. Virtual host.
 */
class provisionConfig_nginx_site extends provisionConfig_nginx {
  public $template = 'vhost.tpl.php';
  public $description = 'nginx site configuration file';

  function filename() {
    return $this->data['nginx_site_conf_path'] . '/' . $this->uri . '_' . $this->site_port;
  }

  function process() {
    if ($this->aliases && !is_array($this->aliases)) {
      $this->aliases = explode(",", $this->aliases);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_vhost_config', $this->uri, $this->data));
  }
}

/**
 * Nginx configuration file class for disabled sites.
 */
class provisionConfig_nginx_site_disabled extends provisionConfig_nginx_site {
  public $template = 'vhost_disabled.tpl.php';
}



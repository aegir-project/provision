<?php

class provisionService_http_apache extends provisionService_http {
  static function option_documentation() {
    return array_merge(parent::option_documentation(), array(
      '--restart_cmd' => 'server with apache: shell command to restart the server; working default will be attepted',
      '--web_ports' => 'server with apache: array of ports to make available; default 80',
    ));
  }

  function init() {
    parent::init();
    $this->apache_site_conf_path = $this->config_path . '/vhost.d';
    $this->apache_platform_conf_path = $this->config_path . '/platform.d';
    $this->apache_conf_path = $this->config_path . '/apache.d';

    // Commands
    $this->setProperty('restart_cmd', _provision_default_restart_cmd());
    $this->setProperty('web_ports', array(80));
  }

  function create_site_config() {
    $config = new provisionConfig_apache_site($this->context, drush_get_context('site'));
    return $config->write();
  }

  function delete_site_config() {
    $config = new provisionConfig_apache_site($this->context);
    return $config->unlink();
  }

  function create_platform_config() {
    $config = new provisionConfig_apache_platform($this->context);
    return $config->write();
  }

  function delete_platform_config() {
    $config = new provisionConfig_apache_platform($this->context);
    return $config->unlink();
  }


  function create_server_config() {
    $config = new provisionConfig_apache_server($this->context);
    return $config->write();
  }

  
  function delete_server_config() {
    $config = new provisionConfig_apache_server($this->context);
    return $config->unlink();
  }

  function verify() {
    switch (d()->type) {
      case 'server':
        d()->service('file')->create_dir($this->apache_site_conf_path, dt("Virtual host configuration"), 0700);
        d()->service('file')->create_dir($this->apache_platform_conf_path, dt("Platforms configuration"), 0700);
        d()->service('file')->create_dir($this->apache_conf_path , dt("Apache configuration"), 0700);

        $this->create_server_config();
        break;

      case 'platform':
        $this->create_platform_config();
        break;

      case 'site':
        $this->create_site_config();
        break;
    }

    d()->service('file')->sync();
    $this->parse_configs();
  }

  function parse_configs($cause_error = FALSE) {
    // This is required to be configurable, due to the fact that different
    // hosts might need to do this differently.
    if (drush_is_local_host(d()->remote_host)) {
      $return = drush_shell_exec(escapeshellcmd(d()->restart_cmd));
    }
    else {
      $return = drush_shell_exec('ssh ' . drush_get_option('ssh-options', '-o PasswordAuthentication=no') . ' %s %s', d()->script_user . '@' . d()->remote_host, escapeshellcmd(d()->restart_cmd));
    }
    if (!$return) {
      $msg = join("\n", drush_shell_exec_output());
      if ($cause_error) {
        return drush_set_error('PROVISION_WEB_RESTART_FAILED', dt("Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)", array("%msg" => $msg)));
      }
      else {
        drush_log(dt("Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)", array("%msg" => $msg)), "warning");
      }
    }
    else {
      drush_log(dt('Apache has been restarted'));
    }

    return $return;
  }


}

class provisionConfig_apache extends provisionConfig {

}


class provisionConfig_apache_server extends provisionConfig_apache {
  public $template = 'server.tpl.php';
  public $description = 'apache server configuration file';

  function filename() {
    return $this->config_path . '/' . 'apache.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

class provisionConfig_apache_platform extends provisionConfig_apache {
  public $template = 'platform.tpl.php';
  public $description = 'apache platform configuration file';

  function filename() {
    return $this->server->apache_platform_conf_path . '/' . ltrim($this->owner, '@') . '.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_dir_config', $this->data));
  }
}

class provisionConfig_apache_site extends provisionConfig_apache {
  public $template = 'vhost.tpl.php';
  public $description = 'apache site configuration file';

  function filename() {
    return $this->platform->server->apache_site_conf_path . '/' . $this->uri . '_' . $this->site_port;
  }

  function process() {
    if ($this->aliases && !is_array($this->aliases)) {
      $this->aliases = explode(",", $this->aliases);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->uri, $this->data));
  }
}


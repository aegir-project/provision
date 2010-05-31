<?php




class provisionService_http_apache extends provisionService_http {
  function init() {
    parent::init();
    $this->setProperty('apache_site_conf_path', $this->config_path . '/vhost.d');
    $this->setProperty('apache_platform_conf_path', $this->config_path . '/platform.d');

    // Commands
    $this->setProperty('restart_cmd', _provision_default_restart_cmd());
  }

  function create_site_config() {
    $config = new provisionConfig_apache_site(drush_get_merged_options());
    return $config->write();
  }

  function delete_site_config() {
    $config = new provisionConfig_apache_site(drush_get_merged_options());
    return $config->unlink();
  }

  function create_platform_config() {
    $config = new provisionConfig_apache_platform(drush_get_merged_options());
    return $config->write();
  }

  function delete_platform_config() {
    $config = new provisionConfig_apache_platform(drush_get_merged_options());
    return $config->unlink();
  }


  function create_server_config() {
    $config = new provisionConfig_apache_server(drush_get_merged_options());
    return $config->write();
  }

  
  function delete_server_config() {
    $config = new provisionConfig_apache_server(drush_get_merged_options());
    return $config->unlink();
  }

  function verify() {
    switch (d($this->owner)->type) {
      case 'server':
        provision_service('file')->create_dir(drush_get_option('apache_site_conf_path'), dt("Virtual host configuration"), 0700);
        provision_service('file')->create_dir(drush_get_option('apache_platform_conf_path'), dt("Platforms configuration"), 0700);
        provision_service('file')->create_dir(drush_get_option('config_path') . '/apache.d' , dt("Apache configuration"), 0700);

        $this->create_server_config();
        break;

      case 'platform':
        $this->create_platform_config();
        break;

      case 'site':
        $this->create_site_config();
        break;
    }

    $this->parse_configs();
  }

  function parse_configs($cause_error = FALSE) {
    // This is required to be configurable, due to the fact that different
    // hosts might need to do this differently.
    if (drush_is_local_host(drush_get_option('host-name', 'localhost'))) {
      $return = drush_shell_exec(escapeshellcmd(drush_get_option('restart_cmd')));
    }
    else {
      $return = drush_shell_exec('ssh ' . drush_get_option('ssh-options', '-o PasswordAuthentication=no') . ' %s %s', drush_get_option('script_user') . '@' . drush_get_option('web_host'), escapeshellcmd(drush_get_option('restart_cmd')));
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
    return $this->data['config_path'] . '/' . 'apache.conf';
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
    return $this->data['platform_conf_path'] . '/platform_' . $this->data['platform-name'] . '.conf';
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
    if (!$this->data['site_port'] || $this->data['site_port'] < 1 || $this->data['site_port'] > 66535) {
      $this->data['site_port'] = 80;
    }

    return $this->data['vhost_path'] . '/' . $this->data['site_url'] . '_' . $this->data['site_port'];
  }

  function process() {
    if ($this->data['aliases'] && !is_array($this->data['aliases'])) {
      $this->data['aliases'] = explode(",", $this->data['aliases']);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->data['site_url'], $this->data));

  }
}


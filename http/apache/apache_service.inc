<?php




class provisionService_http_apache extends provisionService_http {

  function create_site_config($url) {
    $config = new provisionConfig_apache_site(drush_get_merged_options());
    return $config->write();
  }

  function delete_site_config($url) {
    $config = new provisionConfig_apache_site(drush_get_merged_options());
    return $config->unlink();
  }

  function create_platform_config() {
    $config = new provisionConfig_apache_platform(drush_get_merged_options());
    return $config->write();
  }

  function delete_platform_config($url) {
    $config = new provisionConfig_apache_platform(drush_get_merged_options());
    return $config->unlink();
  }


  function create_server_config() {
    $config = new provisionConfig_apache_server(drush_get_merged_options());
    return $config->write();
  }

  
  function delete_server_config($url) {
    $config = new provisionConfig_apache_server(drush_get_merged_options());
    return $config->unlink();
  }


  function verify($url) {
    if (PROVISION_CONTEXT_PLATFORM) {
      provision_service('file')->create_dir(drush_get_option('vhost_path'), dt("Virtual host configuration"), 0700);
      provision_service('file')->create_dir(drush_get_option('platform_conf_path'), dt("Platforms configuration"), 0700);
      
      if (drush_get_option('platform', null)) {
        $this->create_platform_config();
      }

      $this->create_server_config();
    }
    else {
      $this->create_site_config($url);
    }

    $this->parse_configs();
  }

  function parse_configs($cause_error = FALSE) {
    //This is required to be configurable, due to the fact that different hosts might need to do this differently.
    //TODO : test for this instead of relying on a configuration setting?
    $return = drush_shell_exec(escapeshellcmd(drush_get_option('restart_cmd')));
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
    return $this->data['platform_conf_path'] . '/platform_' . $this->data['platform'] . '.conf';
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
    return $this->data['vhost_path'] . '/' . $this->data['site_url'] . '_' . $this->data['site_port'];
  }

  function process() {
    if (!$this->data['site_port'] || $this->data['site_port'] < 1 || $this->data['site_port'] > 66535) { 
      $this->data['site_port'] = 80;
    }
    
    if ($this->data['aliases'] && !is_array($this->data['aliases'])) {
      $this->data['aliases'] = explode(",", $this->data['aliases']);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->data['site_url'], $this->data));

  }
}


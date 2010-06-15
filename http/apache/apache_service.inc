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
    $this->server->apache_site_conf_path = $this->server->config_path . '/vhost.d';
    $this->server->apache_platform_conf_path = $this->server->config_path . '/platform.d';
    $this->server->apache_conf_path = $this->server->config_path . '/apache.d';

    // Commands
    $this->server->setProperty('restart_cmd', _provision_default_restart_cmd());
    $this->server->setProperty('web_ports', array(80));
  }

  /**
   * Pass some extra paths to the config file classes, which specify the server relative paths
   * for use in the filename() methods.
   */
  function config_data() {
    return array(
      'apache_site_conf_path' => $this->server->apache_site_conf_path,
      'apache_platform_conf_path' => $this->server->apache_platform_conf_path,
      'apache_conf_path' => $this->server->apache_conf_path
    );
  }

  function create_site_config() {
    if (d($this->context)->site_enabled) {
      $config = new provisionConfig_apache_site($this->context, $this->config_data() + drush_get_context('site'));
    }
    else {
      $config = new provisionConfig_apache_site_disabled($this->context, $this->config_data() + drush_get_context('site'));
    }

    $config->write();
  }

  function delete_site_config() {
    $config = new provisionConfig_apache_site($this->context, $this->config_data());
    $config->unlink();
  }

  function create_platform_config() {
    $config = new provisionConfig_apache_platform($this->context, $this->config_data());
    return $config->write();
  }

  function delete_platform_config() {
    $config = new provisionConfig_apache_platform($this->context, $this->config_data());
    $config->unlink();
  }

  function create_server_config() {
    provision_file()->create_dir($this->server->apache_site_conf_path, dt("Virtual host configuration"), 0700);
    $this->sync($this->server->apache_site_conf_path);
    provision_file()->create_dir($this->server->apache_platform_conf_path, dt("Platforms configuration"), 0700);
    $this->sync($this->server->apache_platform_conf_path);
    provision_file()->create_dir($this->server->apache_conf_path , dt("Apache configuration"), 0700);
    $this->sync($this->server->apache_conf_path);

    $config = new provisionConfig_apache_server($this->context, $this->config_data());
    return $config->write();
  }

  function delete_server_config() {
    $config = new provisionConfig_apache_server($this->context, $this->config_data());
    $config->unlink();
  }

  function parse_configs() {
    // This is required to be configurable, due to the fact that different
    // hosts might need to do this differently.
    if ($this->server->shell_exec($this->server->restart_cmd)) {
      drush_log(dt('Apache has been restarted'));
    }
    else {
      drush_log(dt('Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)', array('%msg' => join("\n", drush_shell_exec_output()))), 'warning');
    }
  }
}

class provisionConfig_apache extends provisionConfig {
  function write() {
    parent::write();
    $this->owner->service('http')->sync($this->filename());
  }

  function unlink() {
    parent::unlink();
    $this->owner->service('http')->sync($this->filename());
  }
}

/**
 * Apache server level configuration file class
 */
class provisionConfig_apache_server extends provisionConfig_apache {
  public $template = 'server.tpl.php';
  public $description = 'apache server configuration file';

  function filename() {
    return $this->config_path . '/apache.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

/**
 * Apache platform level configuration file class
 */
class provisionConfig_apache_platform extends provisionConfig_apache {
  public $template = 'platform.tpl.php';
  public $description = 'apache platform configuration file';

  function filename() {
    return $this->data['apache_platform_conf_path'] . '/' . ltrim($this->owner->name, '@') . '.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_dir_config', $this->data));
  }
}

/** 
 * Apache site level config class. Virtual host.
 */
class provisionConfig_apache_site extends provisionConfig_apache {
  public $template = 'vhost.tpl.php';
  public $description = 'apache site configuration file';

  function filename() {
    return $this->data['apache_site_conf_path'] . '/' . $this->uri . '_' . $this->site_port;
  }

  function process() {
    if ($this->aliases && !is_array($this->aliases)) {
      $this->aliases = explode(",", $this->aliases);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->uri, $this->data));
  }
}

/**
 * Apache configuration file class for disabled sites.
 */
class provisionConfig_apache_site_disabled extends provisionConfig_apache_site {
  public $template = 'vhost_disabled.tpl.php';
}



<?php

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class provisionService_dns_bind extends provisionService_dns {
  static function option_documentation() {
    return array_merge(parent::option_documentation(), array(
      '--restart_cmd' => 'server with bind: shell command to restart the server; working default will be attepted',
    ));
  }

  function init() {
    parent::init();
    $this->server->bind_conf_path = $this->server->config_path . '/bind.d';
    $this->server->bind_zone_master_path = $this->server->config_path . '/zones/master';
    $this->server->setProperty('bind_restart_cmd', _bind_default_restart_cmd());
  }

  function config_data() {
    return array(
      'bind_conf_path' => $this->server->bind_config_path,
      'bind_zone_master_path' => $this->server->bind_zone_master_path
    );
  }

  function create_server_config() {
    provision_file()->create_dir($this->server->bind_conf_path, dt("Bind configuration"), 0700);
    $this->sync($this->server->bind_conf_path);

    $config = new provisionConfig_bind_server($this->context, $this->config_data());
    $config->write();

  }

  function delete_server_config() {
    $config = new provisionConfig_bind_server($this->context, $this->config_data());
    $config->unlink();
  }

  function parse_configs() {
    // This is required to be configurable, due to the fact that different
    // hosts might need to do this differently.
    if ($this->server->shell_exec($this->server->bind_restart_cmd)) {
      drush_log(dt('Name server on %server has been restarted', array('%server' => $this->server->remote_host)));
    }
    else {
      drush_log(dt('Name server %server could not be restarted. Changes might not be available until this has been done. (error: %msg)', array('%server' => $this->server->remote_host, '%msg' => join("\n", drush_shell_exec_output()))), 'warning');
    }
  }
  
  function create_zone_config() {
    $config = new provisionConfig_bind_master($this->context, $this->config_data() + drush_get_context('zone'));
    $config->write();
  }

}

// XXX: this is an almost extact copy of provisionConfig_apache, we should probably refactor
class provisionConfig_bind extends provisionConfig {
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
 * Bind server-level configuration file
 *
 * This file needs to be included in the global bind configuration file, and Aegir will take it from there
 */
class provisionConfig_bind_server extends provisionConfig_bind {
  public $template = 'server.tpl.php';
  public $description = 'bind server configuration file';

  function filename() {
    return $this->data['server']->config_path . '/bind.conf';
  }
}

/**
 * Bind configuration snippet to include the master zonefiles
 */
class provisionConfig_bind_master extends provisionConfig_bind {
  public $template = 'master.tpl.php';
  public $description = 'bind master configuration file include';

  function filename() {
    return $this->data['bind_conf_path'] . $this->fqdn;
  }
}

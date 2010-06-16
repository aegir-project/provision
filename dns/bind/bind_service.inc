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
    $this->server->bind_conf_path = $this->server->config_path . '/bind.d/';
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

  function commit() {
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

  function create_zone($zone) {
    if (!$this->zone_exists($zone)) {
      $this->create_zonefile($zone, drush_get_option('zone_ttl', $this->server->dns_default_ttl));
      bind_add_line_if_not_exists($this->server->config_path . '/bind.conf', $this->zone_declaration($zone), '/zone\s*"'. $zone . '"/');
    }
    parent::create_zone($zone);
  }

  function zone_declaration($zone) {
    return 'zone "' . $zone . '" { type master; file "' . $this->zone_master_path . '/' . $zone . '"; allow-query { any; }; };';
  }

}


/**
 * add a line to a file if the file (or pattern) is not in the file
 * already
 *
 * This is bound to be called repeatedly so it needs to be fast.
 */
function bind_add_line_if_not_exists($file, $line, $pattern = null) {
  // XXX: we should do exponential backoff here to limit contention
  flock($file);
  $fd = fopen($file);
  $line = trim($line);
  if (is_null($pattern)) {
    $pattern = '/' . $line '/';
  }
  $found = FALSE;
  while ($l = fgets($fd)) {
    if (preg_match($pattern, $l)) {
      $found = TRUE;
      break;
    }
  }
  if (!$found) {
    fseek($fd, SEEK_END);
    fwrite($fd, $line . "\n");
  }
  fclose($fd);
  return $found;
}

/**
 * Bind server-level configuration file
 *
 * This file needs to be included in the global bind configuration file, and Aegir will take it from there
 */
class provisionConfig_bind_server extends provisionConfig {
  public $template = 'server.tpl.php';
  public $description = 'bind server configuration file';

  function filename() {
    return $this->data['server']->config_path . '/bind.conf';
  }
}


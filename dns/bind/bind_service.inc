<?php

function _bind_default_restart_cmd() {
  return "/etc/init.d/named restart";
}

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class provisionService_dns_bind extends provisionService_dns {
  static function option_documentation() {
    return array_merge(parent::option_documentation(), array(
      '--bind_restart_cmd' => 'server with bind: shell command to restart the server; working default will be attepted',
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

  function verify() {
    provision_file()->create_dir($this->server->bind_conf_path, dt("Bind configuration"), 0700);
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
  
  function create_zone($zonename) {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    $zone->add_line_if_not_exists($zone->zone_declaration(), '/zone\s*"'. $zonename . '"/');
    $zone->write();
    parent::create_zone($zonename);
  }

  function add_record($zonename, $name, $destination, $type = 'A') {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    if ($type == 'SOA') { // only one SOA per file
      return $zone->add_line_if_not_exists($name . ' IN SOA ' . $destination, 'SOA');
    } else {
      return $zone->add_line($name . "\tIN\t" . $type . "\t" . $destination);
    }
  }

  function edit_record($zonename, $name, $destination, $type = 'A') {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    if ($type == 'SOA') { // only one SOA per file
      return $zone->add_line_if_not_exists($name . ' IN SOA ' . $destination, 'SOA');
    } else {
      return $zone->replace_line($name . "\tIN\t" . $type . "\t" . $destination, "/^$name\s\s*IN\s\s*$type\s\s*");
    }
  }

}



class provisionConfig_bind_zone extends provisionConfig {
  public $template = 'zone.tpl.php';

  function filename() {
    return $this->server->zone_master_path . '/' . $this->data['name'];
  }

  function exists() {
    return provision_file()->exists($this->filename());
  }

  /**
   * add a line to a file if the file (or pattern) is not in the file
   * already
   *
   * This is bound to be called repeatedly so it needs to be fast.
   */
  function add_line_if_not_exists($line, $pattern = null) {
    $file = $this->filename();
    // XXX: we should do exponential backoff here to limit contention
    flock($file);
    $fd = fopen($file);
    $line = trim($line);
    if (is_null($pattern)) {
      $pattern = '/' . $line . '/';
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

  function zone_declaration() {
    return 'zone "' . $this->data['name'] . '" { type master; file "' . $this->filename() . '"; allow-query { any; }; };';
  }

  
}

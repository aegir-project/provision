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
    $this->server->setProperty('bind_conf_path', $this->server->config_path . '/bind.conf');
    $this->server->setProperty('bind_zone_master_path', $this->server->config_path . '/zones/master');
    $this->server->setProperty('bind_restart_cmd', _bind_default_restart_cmd());
  }

  function config_data() {
    return array(
      'bind_conf_path' => $this->server->bind_config_path,
      'bind_zone_master_path' => $this->server->bind_zone_master_path,
      'bind_restart_cmd' => $this->server->bind_restart_cmd,
    );
  }

  function verify() {
    provision_file()->create_dir($this->server->bind_zone_master_path, dt("Bind zone files"), 0700);
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
    if (!$zone->exists()) {
      drush_log("creating zone");
      //$zone->add_line_if_not_exists($zone->zone_declaration(), '/zone\s*"'. $zonename . '"/');
      $zone->write();

    } else {
      drush_log("zone already exists");
    }
    $this->load_from_zone($zone);
    parent::create_zone($zonename);
  }

  function load_from_zone($zone) {
    // load parameters from zone
    foreach (array('ttl', 'serial', 'refresh', 'retry', 'expire', 'negativettl') as $param) {
      if (isset($zone->data[$param])) {
        drush_log("got param $param from zone, value: " . $zone->data[$param]);
        if ($param == 'serial') {
          $this->$param = provisionService_dns::increment_serial($zone->data['serial']);
        } else {
          $this->$param = $zone->data[$param];
        }
      } else {
        if ($param == 'serial') {
          $this->$param = provisionService_dns::increment_serial();
        } else {
          $longparam = "dns_" . $param;
          $this->$param = $this->server->$longparam;
        }
        drush_log("param $param missing from zone, using default: " . $this->$param);
      }
    }        
  }

  function delete_zone($zonename) {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    if (!$zone->count_records(null, array('NS', 'SOA'))) {
      $zone->delete_file();
    }
  }

  function add_record($zonename, $name, $destination, $type = 'A') {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    if ($type == 'SOA') { // only one SOA per file
      return $zone->add_line_if_not_exists($name . ' IN SOA ' . $destination, '/(?:@\s+)?IN\s+SOA\s+/');
    } else {
      return $zone->add_line($name . "\tIN\t" . $type . "\t" . $destination);
    }
  }

  function edit_record($zonename, $name, $destination, $type = 'A') {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    $pattern = "/^\s*$name\s+IN\s+$type\s+.*$/im";
    if ($type == 'SOA') {
      $pattern = "/^(?:@\s+)?IN\s+SOA\s+[\w.]+\s+[\w.]+\s+\(([^)]*)\)\s*$/ims";
    }
    return $zone->replace_or_add_line($name . "\tIN\t" . $type . "\t" . $destination, $pattern);
  }

  function delete_record($zonename, $name, $destination = null, $type = null) {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    $pattern = "/^$name\s\s*IN\s\s*";
    if (is_null($type)) {
      $pattern .= "\w\w*";
    } else {
      $pattern .= $type;
    }
    $pattern .= "\s\s*";
    if (!is_null($destination)) {
      $pattern .= $destination;
      $pattern .= '$';
    }
    $pattern .= '/';
    return $zone->delete_line($pattern);
  }

  function zone_exists($zone) {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    return $zone->exists();
  }

  function count_records($zone, $include = null, $exclude = null) {
    $zone = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    return $zone->count_records($include, $exclude);
  }

}

class provisionConfig_bind_zone extends provisionConfig {
  public $template = 'zone.tpl.php';

  function parse() {
    $file = $this->filename();
    $body = file_get_contents($file);
    $data = array();
    if (preg_match('/^(?:@\s+)?IN\s+SOA\s+[\w.]+\s+[\w.]+\s+\(([^)]*)\)\s*$/ims', $body, $matches)) {
      $soa = trim($matches[1]);
      $SOA = preg_split('/\s\s*/', $soa);
      $i = 0;
      foreach (array('serial', 'refresh', 'retry', 'expire', 'negativettl') as $param) {
        $data[$param] = $SOA[$i++];
      }
      drush_log("parsed SOA from zonefile:");
      foreach ($data as $key => $value) {
        drush_log($key . ": " . $value);
      }
    } else {
      drush_log("no SOA found in zonefile $file, body: ($body)");
    }
    return $data;
  }

  function filename() {
    return $this->bind_zone_master_path . '/' . $this->data['name'];
  }

  function exists() {
    drush_log("checking file: " . $this->filename());
    return provision_file()->exists($this->filename())->status();
  }

  /**
   * This returns the number of records in the zone.
   */
  function count_records($include = null, $exclude = null) {
    return TRUE;
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
    $fd = fopen($file, 'r+'); // read/write, beginning of file
    flock($fd, LOCK_EX);
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
      fseek($fd, 0, SEEK_END);
      drush_log("pattern $pattern not found in file $file, adding at the end");
      fwrite($fd, $line . "\n");
    }
    fclose($fd);
    return $found;
  }

  /**
   * add a line to a file if the file (or pattern) is not in the file
   * already
   *
   * This is bound to be called repeatedly so it needs to be fast.
   */
  function add_line($line) {
    $file = $this->filename();
    // XXX: we should do exponential backoff here to limit contention
    $fd = fopen($file, 'a'); // read/write, beginning of file
    flock($fd, LOCK_EX);
    $line = trim($line);
    fwrite($fd, $line . "\n");
    fclose($fd);
  }

  function replace_or_add_line($replacement, $pattern) {
    $file = $this->filename();
    $fd = fopen($file, 'r+');
    flock($fd, LOCK_EX);
    $body = fread($fd, filesize($file));
    $newbody = preg_replace($pattern, $replacement, $body);
    if ($body === $newbody) {
      drush_log("pattern $pattern not found in $file, adding at the end");
      fseek($fd, 0, SEEK_END);
      fwrite($fd, $replacement . "\n");
    } else {
      drush_log("pattern $pattern found and replaced with $replacement");
      fseek($fd, 0);
      fwrite($fd, $newbody);
      ftruncate($fd, strlen($newbody));
    }
    fclose($fd);    
  }

  function zone_declaration() {
    return 'zone "' . $this->data['name'] . '" { type master; file "' . $this->filename() . '"; allow-query { any; }; };';
  }

  
}

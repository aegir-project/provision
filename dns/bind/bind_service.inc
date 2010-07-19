<?php

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class provisionService_dns_bind extends provisionService_dns {
  protected $application_name = 'bind';
  protected $has_restart_command = TRUE;
  private $zone_cache = array();
  
  static function bind_default_restart_cmd() {
    return "/etc/init.d/named restart";
  }

  function default_restart_cmd() {
    return provisionService_dns_bind::bind_default_restart_cmd();
  }

  function init() {
    parent::init();
    $this->server->setProperty('bind_conf_path', $this->server->config_path . '/bind.conf');
    $this->server->setProperty('bind_zone_master_path', $this->server->config_path . '/zones/master');
    $this->configs['server'][] = 'provisionConfig_bind_master_zone';
  }

  function config_data($config, $class) {
    $data = parent::config_data($config, $class);
    $data['bind_conf_path'] = $this->server->bind_config_path;
    $data['bind_zone_master_path'] = $this->server->bind_zone_master_path;
    return $data;
  }

  function verify() {
    if ($this->context->type =='server') {
      provision_file()->create_dir($this->server->bind_zone_master_path, dt("Bind zone files"), 0700);
    }
  }

  /**
   * Restart the server to pick up the new configuration files.
   */
  function commit() {
    // the restart_cmd stuff is part of the base server class now.
    $this->restart();
  }

  function create_zone($zonename) {
    $zone = $this->zone_singleton($zonename);
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
    $zone = $this->zone_singleton($zonename);
    if (!$zone->count_records(null, array('NS', 'SOA'))) {
      $zone->delete_file();
    }
  }

  function add_record($zonename, $name, $type, $destination) {
    $zone = $this->zone_singleton($zonename);
    if ($type == 'SOA') { // only one SOA per file
      return $zone->add_line_if_not_exists($name . ' IN SOA ' . $destination, '/(?:@\s+)?IN\s+SOA\s+/');
    } else {
      return $zone->add_line($name . "\tIN\t" . $type . "\t" . $destination);
    }
  }

  function edit_record($zonename, $name, $type, $destination) {
    drush_log("edit: $zonename, $name, $type, $destination");
    $zone = $this->zone_singleton($zonename);
    $pattern = "/^\s*$name\s+IN\s+$type\s+.*$/im";
    if ($type == 'SOA') {
      $pattern = "/^(?:@\s+)?IN\s+SOA\s+[\w.]+\s+[\w.]+\s+\(([^)]*)\)\s*$/ims";
    }
    return $zone->replace_or_add_line($name . "\tIN\t" . $type . "\t" . $destination, $pattern);
  }

  function delete_record($zonename, $name, $type = null, $destination = null) {
    $zone = $this->zone_singleton($zonename);
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

  function zone_exists($zonename) {
    $zone = $this->zone_singleton($zonename);
    return $zone->exists();
  }

  function count_records($zonename, $include = null, $exclude = null) {
    $zone = $this->zone_singleton($zonename);
    return $zone->count_records($include, $exclude);
  }

  public function &zone_singleton($zonename) {
    if (!$this->zone_cache[$zonename]) {
      $this->zone_cache[$zonename] = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    }
    return $this->zone_cache[$zonename];
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

}

class provisionConfig_bind_master_zone {
  function filename() {
    return $this->bind_conf_path;
  }

  function zone_declaration() {
    return 'zone "' . $this->data['name'] . '" { type master; file "' . $this->bind_zone_master_path . '/' . $this->data['name'] . '"; allow-query { any; }; };';
  }

}

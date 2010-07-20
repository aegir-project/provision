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
    }
    return parent::create_zone($zonename);
  }

  function delete_zone($zonename) {
    $zone = $this->zone_singleton($zonename);
    if (!$zone->count_records(null, array('NS', 'SOA'))) {
      $zone->delete_file();
    }
  }

  function add_record($zonename, $name, $type, $destination) {
    drush_log("add: $zonename, $name, $type, $destination");
    
    $zone = $this->zone_singleton($zonename);
    if ($type == 'SOA') { // only one SOA per file
      foreach ($zone->records as $record) {
        if ($record['type'] == 'SOA') {
          $record['destination'] = $destination;
        }
      }
    } else {
      $zone->records[] = array('name' => $name, 'type' => $type, 'destination' => $destination);
    }
    return $zone->write();
  }

  function edit_record($zonename, $name, $type, $destination = NULL) {
    drush_log("edit: $zonename, $name, $type, $destination");
    $zone = $this->zone_singleton($zonename);
    $found = FALSE;
    foreach ($zone->records as $key => $record) {
      if ($record['name'] == $name && $record['type'] == $type) {
        if ($found) {
          unset($zone->records[$key]);
        } else {
          $record['destination'] = $destination;
          $found = TRUE;
        }
      }
    }
    if (!$found) {
      $this->add_record($zonename, $name, $type, $destination);
    }
    return $zone->write();
  }

  function delete_record($zonename, $name, $type = null, $destination = null) {
    $zone = $this->zone_singleton($zonename);

    foreach ($this->records as $key => $record) {
      if ($record['name'] == $name) {
        if ( ( is_null($type) || $record['type'] == $type) 
             && ( is_null($destination) || $record['destination'] == $destination) ) {
          unset($zone->records[$key]);
        }
      }
    }
    return $zone->write();
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
      if ($this->context->type == 'site') {
        $this->context = '@server_master';
        drush_log("overriding server context");
      }
      $this->zone_cache[$zonename] = new provisionConfig_bind_zone($this->context, array('name' => $zonename));
    }
    return $this->zone_cache[$zonename];
  }

}

class provisionConfig_bind_zone extends provisionConfig {
  public $template = 'zone.tpl.php';
  public $records = array();

  function parse() {
    $file = $this->filename();
    $body = file_get_contents($file);
    if ($body === FALSE) {
      drush_log("no zonefile fonud in " . $file);;
      return array();
    }
    $matches = array();
    if ($this->record_count = preg_match_all('/^((?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.?)*|@)?\s*IN\s+(\w+)\s+(.*)\s*$/im', $body, $matches, PREG_SET_ORDER)) {
      drush_log(dt("parsed zonefile %file, found %count records", array('%file' => $file, '%count' => $this->record_count)));
    } else {
      drush_log(dt("could not parse existing zonefile %file", array('%file' => $file)));
    }
    $SOA = FALSE;
    foreach ($matches as $match) {
      $name = trim($match[1]);
      $type = strtoupper(trim($match[2]));
      $destination = trim($match[3]);
      switch ($type) {
      case 'SOA':
        $SOA = TRUE;
      default:
        drush_log("found record: $name:$type:$destination");
        $this->records[] = array('name' => $name, 'type' => $type, 'destination' => $destination);
        break;
      }
    }
    if (!$SOA) {
      drush_log("no SOA found in zonefile $file, body: ($body)", "warning");
    }
    return $this->records;
  }

  function filename() {
    return $this->bind_zone_master_path . '/' . $this->data['name'];
  }

  function exists() {
    if (!$this->filename()) {
      drush_log("filename null??");
      return FALSE;
    }
    return provision_file()->exists($this->filename())->status();
  }

  /**
   * This returns the number of records in the zone.
   */
  function count_records($include = null, $exclude = null) {
    return TRUE;
  }

  static function parse_soa_data($destination) {
    if (preg_match('/([\w.]+)\s+([\w.]+)\s+\(([^)]*)\)/', $destination, $matches)) {
      drush_log(dt('parsed correctly SOA record "%data"', array("%data" => $matches[3])));
    } else {
      drush_log(dt('irregular SOA record: %data', array('%data' => $destination)));
      return FALSE;
    }
    $soa_data = array('name' => $matches[1], 'email' => $matches[2]);
    $SOA = preg_split('/\s\s*/', trim($matches[3]));
    $i = 0;
    foreach (array('serial', 'refresh', 'retry', 'expire', 'negativettl') as $param) {
      $soa_data[$param] = $SOA[$i++];
    }
        
    foreach ($soa_data as $key => $value) {
      drush_log($key . ": " . $value);
    }
    return $soa_data;
  }

  function gen_soa_data($soa_data = null) {
    if (empty($soa_data['name'])) {
      $soa_data['name'] = $this->data['name'];
    }
    if (empty($soa_data['email'])) {
      $soa_data['email'] = drush_get_option('zone_hostmaster', 'hostmaster.'. $this->data['name'] . '.');
    }
    $destination = $soa_data['name'] . " "; // FQDN
    $destination .= $soa_data['email']; // email
    $destination .= ' ( ';
    $new_serial = provisionService_dns::increment_serial($soa_data['serial']);
    drush_log(dt("incremented serial from %old to %new", array("%old" => $soa_data['serial'], "%new" => $new_serial)));
    $destination .= $new_serial;
    foreach (array('refresh', 'retry', 'expire', 'negativettl') as $param) {
      if (isset($soa_data[$param])) {
        $destination .= $soa_data[$param] . ' ';
      } else {
        $destination .= $this->data['server']->$param;
      }
    }
    $destination .= ")";
    return $destination;
  }

  function write() {
    foreach ($this->records as &$record) {
      if ($record['type'] == 'SOA') {
        $record['destination'] = $this->gen_soa_data($this->parse_soa_data($record['destination']));
        drush_log("changed SOA to " . $record['destination']);
        $SOA = TRUE;
      }
    }
    if (!$SOA) {
      array_unshift($this->records, array('name' => '@', 'type' => 'SOA', 'destination' => $this->gen_soa_data()));
    }
    $zonefile = fopen($this->filename() . ".tmp", "a");
    flock($zonefile, LOCK_EX);
    foreach($this->records as $record) {
      fwrite($zonefile, $record['name'] . "\tIN\t" . $record['type'] . "\t" . $record['destination'] . "\n");
    }
    fflush($zonefile);
    // XXX: test config here before renaming, rollback on errors
    rename($this->filename() . ".tmp", $this->filename());
    fclose($zonefile); // this also releases the lock
    drush_log(dt("wrote zonefile %file", array("%file" => $this->filename())));
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

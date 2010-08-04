<?php

/**
 * Implementation of the DNS service through BIND9
 *
 * A lot of this is inspired by the Apache implementation of the HTTP service.
 */
class provisionService_dns_bind extends provisionService_dns_complex {
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
    $this->configs['server'][] = 'provisionConfig_bind_server';
    $this->configs['zone'][] = 'provisionConfig_bind_zone';
  }

  function parse_configs() {
    $this->restart();
  }
}

class provisionConfig_bind_zone extends provisionConfig_dns_zone {
  function filename() {
    return $this->dns_zoned_path . '/' . $this->data['name'];
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
      if (empty($soa_data[$param])) {
        drush_log("defaulting to server default " . $this->server);
        $destination .= $this->data['server']->$param;
      } else {
        $destination .= $soa_data[$param] . ' ';
      }
    }
    $destination .= ")";
    return $destination;
  }

  function write() {
    foreach ($this->records as $key => $record) {
      if ($record['type'] == 'SOA') {
        if ($SOA) {
          drush_log(dt("removing duplicate SOA record %data", array('%data' => var_export($this->records[$key], true))), 'warning');
          unset($this->records[$key]);
        } else {
          $this->records[$key]['destination'] = $this->gen_soa_data($this->parse_soa_data($record['destination']));
          $SOA = TRUE;
        }
      }
    }
    if (!$SOA) {
      array_unshift($this->records, array('name' => '@', 'type' => 'SOA', 'destination' => $this->gen_soa_data()));
    }
    $zonefile = fopen($this->filename() . ".tmp", "a");
    flock($zonefile, LOCK_EX);
    foreach($this->records as $record) {
      $str = $record['name'] . "\tIN\t" . $record['type'] . "\t" . $record['destination'] . "\n";
      fwrite($zonefile, $str);
    }
    fflush($zonefile);
    // XXX: test config here before renaming, rollback on errors
    rename($this->filename() . ".tmp", $this->filename());
    fclose($zonefile); // this also releases the lock
    drush_log(dt("wrote zonefile %file", array("%file" => $this->filename())));
  }
}

class provisionConfig_bind_server extends provisionConfig_dns_server {
}

<?php
// $Id: dns.drush.inc,v 1.4 2009/03/20 16:13:24 adrian Exp $
/**
 * @file
 *    DNS provisioning module.
 *
 * The goal of this module is to manage DNS zonefiles and Resource Records
 * (RRs), for sites that are about to be created.  It uses the provision API to
 * tie into the right places in the site creation work flow.
 */

include_once(dirname(__FILE__) . '/../provision.service.inc');

/**
 * Implementation of hok_drush_command().
 */
function dns_drush_command() {
  $items['provision-zone'] = array(
    'arguments' => array('operation' => dt('The operation to perform on a zone (verify, delete, rr-add, rr-delete)')),
    'description' => dt('Manipulate a zonefile'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  return $items;
}

function drush_dns_provision_zone($action, $zone, $name = null, $type = null, $destination = null) {
  switch ($action) {
  case 'create-host':
    $status = d()->service('dns')->create_host($zone);
    break;
  case 'delete-host':
    $status = d()->service('dns')->delete_host($zone);
    break;
  case 'verify':
  case 'create':
    $status = d()->service('dns')->create_zone($zone);
    break;
  case 'delete':
    $status = d()->service('dns')->delete_zone($zone);
    break;
  case 'rr-add':
    $status = d()->service('dns')->add_record($zone, $name, $type, $destination);
    break;
  case 'rr-modify':
    $status = d()->service('dns')->modify_record($zone, $name, $type, $destination);
    break;
  case 'rr-delete':
    $status = d()->service('dns')->delete_record($zone, $name, $type, $destination);
    break;
  }
  $status = $status && d()->service('dns')->commit($zone);
  
  return $status;
}


function dns_provision_services() {
  return array('dns' => NULL);
}

class provisionService_dns extends provisionService {
  public $service = 'dns';

  static function option_documentation() {
    return array(
      
    );
  }

  function init() {
    $this->server->setProperty('dns_ttl', 86400); # 24h
    $this->server->setProperty('dns_refresh', 21600); # 6h
    $this->server->setProperty('dns_retry', 3600); # 1h
    $this->server->setProperty('dns_expire', 604800); # 7d
    $this->server->setProperty('dns_negativettl', 86400); # 24h
  }


  function verify() {
    switch ($this->context->type) {
      case 'server' :
        $this->create_server_config();
        break;
      case 'site' :
        $this->create_host();
        break;
    }

    $this->parse_configs();
  }

  function create_server_config() {
    return FALSE;
  }

  function delete_server_config() {
   return FALSE;
  }

  /**
   * Determine if a zone is defined on the nameserver.
   *
   * To be implemented by engines.
   *
   * @return if the zone exists
   */
  function zone_exists($zone) {
    return FALSE;
  }

  /**
   * This returns the number of records in the zone.
   */
  function count_records($zone, $include = null, $exclude = null) {
    return FALSE;
  }

  /**
   * Guess in which zone we should create the record
   *
   * This function will examine the existing zones to find to which
   * this host belongs to.
   *
   * @param $host the name of the record to add (e.g. www.example.com)
   *
   * @returns array the record and zone name to add the record to (e.g. www and example.com)
   */
  function guess_zone($host) {
    $tld = $host;
    $parts = explode(".", $host);
    $subdomain = array();
    $found = FALSE;
    drush_log("zone $tld, found: $found, parts: " . count($parts));
    while (!$found && (count($parts) >= 2)) {
      $tld = join(".", $parts);
      if ($this->zone_exists($tld)) {
        $found = TRUE;
      } else {
        $scrap = array_shift($parts);
        $subdomain[] = $scrap;
        drush_log("zone $tld not found, ditching $scrap, count: " . count($parts));
        $found = FALSE;
      }
    }
    $subdomain = join(".", $subdomain);
    if (!$subdomain) {
      $subdomain = '@';
    }
    drush_log("guess_zone guessed parts $tld, $subdomain");
    return array($subdomain, $tld);
  }

  function soa_string($name) {
    $string = $name . ". "; # the FQDN
    $string .= drush_get_option('zone_hostmaster', 'hostmaster.'. $name . '.'); # the email
    $string .= " ( ";
    $string .= $this->serial . " ";
    $string .= $this->refresh . " ";
    $string .= $this->retry . " ";
    $string .= $this->expire . " ";
    $string .= $this->negativettl . " ";
    $string .= " ) ";
    return $string;
  }

  /**
   * Helper function to increment a zone's serial number.
   *
   * @param $serial
   *    A serial in YYYYMMDDnn format. If null, a new serial based on
   *    the date will be generated.
   *
   * @return
   *    The serial, incremented based on current date and index
   */
  function increment_serial($serial = null) {
    $date = substr($serial, 0, 8); # Get the YYYYMMDD part
    $today = date('Ymd');
    if (is_null($serial) || $date != $today) {
      return $today . '00';
    } else {
      $index = substr($serial, 8, 2); # Get the index part
      if ($index >= 99) {
        drush_set_error("serial number overflow");
      } else {
        $index++;
      }
      return $date . sprintf('%02d', $index);
    }
  }

  /**
   * This creates a zone, which mostly consists of adding the SOA record.
   */
  function create_zone($fqdn = null) {
    $this->edit_record($fqdn, '@', 'SOA', $this->soa_string($fqdn));
  }

  /**
   * This completely drops a zone, without any checks.
   */
  function delete_zone($fqdn) {
    return FALSE;
  }

  /**
   * This adds a (potentially) duplicate record (RR) in a zonefile
   *
   * This doesn't check if the record already exists, it just adds it
   * to the end of the file.
   */
  function add_record($zonename, $name, $type, $destination) {
    return FALSE;
  }

  /**
   * This creates or replaces a record (RR) in a zonefile.
   */
  function edit_record($zonename, $name, $type, $destination) {
    return FALSE;
  }

  /**
   * This removes a record matching the name and destination.
   *
   * If destination or type is null, they are disregarded in the pattern.
   */
  function delete_record($zonename, $name, $type, $destination = null) {
    return FALSE;
  }

  /** 
   * Create a host in DNS.
   *
   * This can do a lot of things, create a zonefile, add a record to a
   * zonefile, it's going to make its best guess doing the Right
   * Thing.
   *
   * @arg $host string the hostname to create. If null, we look in the
   * current context (should be a site) for a URI.
   */
  function create_host($host = null) {
    if (is_null($host)) {
      $host = $this->context->uri;
    }
    $parts = $this->guess_zone($host);
    $zone = $parts[1];
    $sub = $parts[0];
    if (!$this->zone_exists($zone)) {
      drush_log("in create_host, zone not found");
      $this->create_zone($zone);
    }
    $ips = d($site)->service('http')->server->ip_addresses;
    if (!$ips && count($ips) < 1) {
      drush_log(dt("no IP found for server, trying loopback"));
      $ips = '127.0.0.1';
    }
    if (is_array($ips)) {
      drush_log("a bunch of ips defined, deleting existing ones and readding: " . join(",", $ips));
      $this->delete_record($zone, $sub, 'A');
      foreach ($ips as $ip) {
        $this->add_record($zone, $sub, 'A', $ip);
      }
    } else {
      $this->edit_record($zone, $sub, 'A', $ips);
    }
  }

  /**
   * Delete a host from DNS
   *
   * Similar to create host, this will seek and destroy that host throughout zonefiles.
   *
   * @arg $host string the hostname to create. If null, we look in the
   * current context (should be a site) for a URI.
   */
  function delete_host($host = null) {
    if (is_null($host)) {
      $host = $this->context->uri;
    }
    $parts = $this->guess_zone($host);
    $this->delete_record($parts[0], $parts[1], 'A');
    if ($this->count_records($parts[0], null, array('NS', 'SOA'))) {
      $this->delete_zone($parts[0]);
    }

  }

  /**
   * Commit changes to the DNS server
   *
   * This may involve restarting the server.
   *
   * @arg $host only reload a single zone. if null, reload all zones
   */
  function commit($zone = null) {
    return FALSE;
  }
}

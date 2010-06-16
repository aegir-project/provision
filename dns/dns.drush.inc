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

function drush_dns_provision_zone($action, $zone) {
  switch ($action) {
  case 'verify':
  case 'create':
    $status = d()->service('dns')->create_zone($zone);
    break;
  case 'delete':
    $status = d()->service('dns')->delete_zone($zone);
    break;
  case 'rr-add':
  case 'rr-delete':
    drush_set_error("not implemented");
    break;
  }
  if ($status) {
    _bind_reload($zone->origin);
  }
}


/**
 * Helper function to increment a zone's serial number.
 *
 * @param $serial
 *    A serial in YYYYMMDDnn format
 *
 * @return
 *    The serial, incremented based on current date and index
 */
function _dns_increment_serial($serial) {
  $date = substr($serial, 0, 8); # Get the YYYYMMDD part
  $index = substr($serial, 8, 2); # Get the index part
  $today = date('Ymd');
  if ($date == $today) {
    return $date . sprintf('%02d', $index+1);
  } else {
    return $today . '01';
  }
}

function dns_provision_services() {
  return array('dns' => NULL);
}

class provisionService_dns extends provisionService {
  static function option_documentation() {
    return array(
      
    );
  }

  function init() {

  }


  function verify() {
    return $this->create_server_config();
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
   */
  function guess_zone($host) {
    if (zone_exists($host)) {
      return array($host, '@');
    } else {
      // TODO: examine subdomains
      return array($host, '@');
    }
  }

  function soa_string($name) {
    $string = $name . ". "; # the FQDN
    $string .= drush_get_option('zone_hostmaster', 'hostmaster.'. $name . '.'); # the email
    $string .= " ( ";
    $string .= drush_get_option('zone_serial', $this->increment_serial()) . " ";
    $string .= drush_get_option('zone_refresh', $this->server->dns_default_refresh) . " ";
    $string .= drush_get_option('zone_retry', $this->server->dns_default_retry) . " ";
    $string .= drush_get_option('zone_expire', $this->server->dns_default_expire) . " ";
    $string .= drush_get_option('zone_negativettl', $this->server->dns_default_negativettl) . " ";
    $string .= " ) ";
  }

  /**
   * This creates a zone, which mostly consists of adding the SOA record.
   */
  function create_zone($fqdn) {
    $this->create_record('@', $this->soa_string($fqdn), 'SOA');
  }

  /**
   * This completely drops a zone.
   */
  function delete_zone($fqdn) {
    return FALSE;
  }

  /**
   * This creates a record (RR) in a zonefile.
   */
  function create_record($name, $destination, $type = 'A', $pri = '') {
    return FALSE;
  }

  /** 
   * Create a host in DNS.
   *
   * This can do a lot of things, create a zonefile, add a record to a
   * zonefile, it's going to make its best guess doing the Right Thing.
   */
  function create_host($host) {
    $parts = $this->guess_zone($host);
    if (!$this->zone_exists($parts[0])) {
      $this->create_zone($parts[0]);
    }
    $this->create_record($parts[1]);
  }

  /**
   * Delete a host from DNS
   *
   * Similar to create host, this will seek and destroy that host throughout zonefiles.
   */
  function delete_host($host) {
    $parts = $this->guess_zone($host);
    $this->delete_record($parts[1]);
    if ($this->count_records(null, array('NS', 'SOA'))) {
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

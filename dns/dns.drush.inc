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
   * Create a host in DNS.
   *
   * This can do a lot of things, create a zonefile, add a record to a
   * zonefile, it's going to make its best guess doing the Right Thing.
   */
  function create_host($host) {
    return FALSE;
  }

  /**
   * Delete a host from DNS
   *
   * Similar to create host, this will seek and destroy that host throughout zonefiles.
   */
  function delete_host($host) {
    return FALSE;
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

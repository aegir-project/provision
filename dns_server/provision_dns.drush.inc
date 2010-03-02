<?php
// $Id: provision_dns.drush.inc,v 1.4 2009/03/20 16:13:24 adrian Exp $
/**
 * @file
 *    DNS provisioning module.
 *
 * The goal of this module is to manage DNS zonefiles and Resource Records
 * (RRs), for sites that are about to be created.  It uses the provision API to
 * tie into the right places in the site creation work flow.
 */

# BIND engine is the only one (for now)
include_once('provision_dns.bind.inc');

/**
 * Implementation of hook_drush_command().
 */
function provision_dns_drush_command() {

  $items['provision zone'] = array(
    'arguments' => array('domain.com' => dt('The domain of the zone name to configure.')),
    'description' => dt('Setup a zonefile'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  return $items;
}

/**
 * Helper function for creating a new zone via the provision_dns_zone API function.
 *
 * @param $domain
 *   The domain name/origin for the zone to be created.
 *
 * @return
 *   The zone object, whether newly created, or already existing.
 **/
function provision_dns_create_zone($domain) {

  $writable = provision_path("writable", PROVISION_NAMED_PATH, TRUE, NULL,
                             t("DNS configuration path @path is not writable"),
                             PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR);
  drush_log("Notice", dt("Generating zone file for url @url", array('@url' => $domain)));
  provision_dns_zone('add', $zone_fields);
  foreach ($ns_recs as $ns) {
    provision_dns_create_record('@', $zone, array('type' => 'NS', 'dns_ip' => trim($ns)));
  }
}

/**
 * Helper function for creating a new A record via the provision_dns_rr API function.
 *
 * @param $host
 *   The name part of the record to be created.
 * @param $zone
 *   The zone object in which to create the record.
 *
 * @return
 *   The record object, or FALSE if the record could not be created.
 **/
function provision_dns_create_record($host, $zone) {
  drush_log(dt("Creating %type record for url @url", array('%type' => drush_get_option('type'), '@url' => $host)));
  $rec['name'] = $host;
  $rec['type'] = drush_get_option('type');
  $rec['data'] = drush_get_option('dns_ip');
  return provision_dns_rr('add', $zone->zid, $rec);
}

/**
 * Helper function for removing a given URL's record as well as zonefile (if it's the last
 * record in the zone) via the provision_dns_zone and provision_dns_rr API functions.
 *
 * @param $url
 *   The URL to remove. The zone and RR will be extracted from this and removed via the API.
 **/
function provision_dns_delete($url) {
  # do i need to resurrect this?
  $url_parts = _provision_dns_split_url($url);

# These two objects should now be passed through from the hosting layer..j
#  if ($zone = provision_dns_status('zone', array('domain' => $url_parts['domain']))) {
#    if ($rec = provision_dns_status('rr', array('zid' => $zone->zid, 'name' => $url_parts['host'], 'type' => 'A'))) {

      # delete the A record for this site..
      provision_dns_rr('delete', $zone, array('rid' => $rec->rid));

      # and (as long as it's the only host on this domain) delete the zone as well
#      if (provision_dns_getrecords($zone->zid, TRUE) <= 1) {
	provision_dns_zone('delete', array('zid' => $zone->zid));
#      }
#    }
#  }
}


/**
 * Manage zones
 *
 * @param $op
 *   The operation to be done on the zone. Valid $ops are 'add', 'delete' and 'update'.
 * @param $args
 *   An array which identifies the zone (using the 'origin' or 'zid' keys) plus data for
 *   use in the operation itself. Valid data keys for $args are:
 *   'dns_id', 'zid', 'origin', 'ns1', 'ns2', 'mbox', 'serial', 'refresh', 'retry', 'expire', 'minimum', 'ttl'
 *   The special 'dns_id' key causes the function to load a dns_server node with this ID for use in populating
 *   defaults for the other fields.
 *
 * @return
 *   For the 'add' and 'update' $ops, returns the zone object.
 **/
function provision_dns_zone($op, $args) {
  $fields = array('origin', 'ns1', 'ns2', 'mbox', 'serial', 'refresh', 'retry', 'expire', 'minimum', 'ttl', 'active', 'xfer');
  if (isset($args['dns_id'])) { $dns = node_load($args['dns_id']); }
  // do whatever DB setup is necessary, then call hook_dns_zone($op, $args) to let the engine do the work..
  switch ($op) {
    case 'add':
# Again, the zone object needs to be passed to the API
#      if ($zone = provision_dns_status('zone', array('domain' => $args['origin']))) {
#        // domain already exists, just return the zid
#        return $zone->zid;
#      } else {
        if (!isset($args['origin'])) {
          // error, need at least this arg
          return FALSE;
        }
        // build query, and INSERT and set the $zone object
        foreach ($fields as $field) {
          if (isset($args[$field])) {
            $f[] = $field;
            $v[] = $args[$field];
          } else {
            # Use default from dns_server node
            if (isset($dns)) {
	      $f[] = $field;
	      $v[] = $dns->$field;
            }
          }
        }
# TODO: move this up into hosting, and pass the zone object down
#	$sql = sprintf("INSERT INTO {provision_dns_soa} (%s) VALUES ('%s')",
#			implode(",", $f),
#			implode("','", $v));
#        db_query($sql);
#        $zone = provision_dns_status('zone', array('domain' => $args['origin']));
#      }
# This should call the engine..
#      module_invoke_all("provision_dns_zone", $op, $zone);
      if ($dns) {  # Add a default A record for the origin
        provision_dns_rr('add', $zone, array('name' => '@', 'type' => 'A', 'data' => $dns->default_ip));
      }
      break;
    case 'update':
# Here again, this object should come from hosting..
#      if (!isset($args['zid'])) {
#        $zone = provision_dns_status('zone', array('domain' => $args['origin']));
#      } else {
#        $zone = provision_dns_status('zone', array('zid' => $args['zid']));
#      }

#      if ($zone || provision_dns_status('zone', $args)) {
        // build update query
        $keys = array();
        if (!isset($args['serial']) || $zone->serial == $args['serial']) {
          $args['serial'] = _provision_dns_increment_serial($zone->serial);
        }
        foreach ($fields as $field) {
          if (isset($args[$field]) && $args[$field] != $zone->$field) {
            if ($field == 'origin') {
              $zone->old_origin = $zone->origin; # Save this so engines can modify name
            }
            $keys[] = $field . "= '%s'";
            $values[] = $args[$field];
            $zone->$field = $args[$field];
          }
        }
        $values[] = $zone->zid;

        if (!empty($keys)) {
# TODO: move this up into hosting, and pass the zone object down
#	  $sql = "UPDATE {provision_dns_soa} SET ". implode(',', $keys) ." WHERE zid=%d";
#	  db_query($sql, $values);
        }
#      } else {
#        // zone doesn't exist, return error?
#        return;
#      }
# Call the engine..
#      module_invoke_all("provision_dns_zone", $op, $zone);
      return $zone;
      break;
    case 'delete':
#      $zone = provision_dns_status('zone', $args);
# invoke engine
#      module_invoke_all('provision_dns_zone', 'delete', $zone);  # This will delete all the zone's records too

# TODO:
#      db_query('DELETE FROM {provision_dns_rr} WHERE zid = %d', $zone->zid);
#      db_query('DELETE FROM {provision_dns_soa} WHERE zid = %d', $zone->zid);
      break;
  }

}

/**
 * Add, delete, or update Resource Records (see http://www.dns.net/dnsrd/rr.html and http://www.bind9.net/dns-parameters)
 *
 * @param $op
 *   What type of operation to execute on the zone: add, delete, or update.
 * @param $zone
 *   The zone object in which the RR resides.
 * @param $args
 *   An array identifying the RR to operate on plus data for use in the operation itself.
 *   Valid keys for this array are:
 *   'name', 'type', 'data', 'aux', 'ttl'
 *
 * @return
 *   For the 'add' and 'update' $ops, return the record object.
 **/
function provision_dns_rr($op, $zone, $args) {
  $fields = array('name', 'type', 'data', 'aux', 'ttl');
#  if (is_numeric($zone)) {
#    $zone = provision_dns_status('zone', array('zid' => $zone));
#  }
  if (!is_object($zone)) {
    return FALSE;  # Error, the zone doesn't seem to exist!
  }

  // do whatever DB setup is necessary, then call hook_dns_rr($op, $args)
  switch ($op) {
    case 'add':
#      if ($rr = provision_dns_status('rr', array_merge(array('zid' => $zone->zid), $args))) {
#	// RR already exists
#	return $rr;
#      }
      if (!isset($args['name']) || !isset($args['type']) || !isset($args['data'])) {  # Not enough info
	return FALSE;
      }
# TODO:
#      $sql = sprintf("INSERT INTO {provision_dns_rr} (zid, name, type, data, aux, ttl) VALUES (%d, '%s', '%s', '%s', %d, %d)",
#		     $zone->zid, $args['name'], $args['type'], $args['data'], $args['aux'], $args['ttl']);
#      db_query($sql);
#      $record = provision_dns_status('rr', array('zid' => $zone->zid, 'name' => $args['name'], 'type' => $args['type']));
# Call the engine..
#      module_invoke_all('provision_dns_rr', $op, $zone, $record);
      provision_dns_zone('update', array('zid' => $zone->zid, 'serial' => _provision_dns_increment_serial($zone->serial)));
      return $record;
      break;
    case 'update':
# passed in via args?
#      $record = provision_dns_status('rr', array('zid' => $zone->zid, 'rid' => $args['rid']));

      $keys = array();
      foreach ($fields as $field) {
        if (isset($args[$field]) && $args[$field] != $record->$field) {
          if ($field == 'name') { $record->old_name = $record->name; } # Save this so engines can modify it
          $values[] = $args[$field];
          $record->$field = $args[$field];
          if ($field == 'aux' || $field == 'ttl') { # numeric
            $keys[] = $field . "= %d";
          } else { # string
            $keys[] = $field . "= '%s'";
          }
        }
      }
      $values[] = $zone->zid;
      $values[] = $args['rid'];

      if (!empty($keys)) {
# TODO:
#	$sql = "UPDATE {provision_dns_rr} SET ". implode(',', $keys) ." WHERE zid=%d AND rid=%d";
#	db_query($sql, $values);
      }
# Call the engine
#      module_invoke_all('provision_dns_rr', $op, $zone, $record);
      provision_dns_zone('update', array('zid' => $zone->zid, 'serial' => _provision_dns_increment_serial($zone->serial)));
      return $record;
      break;
    case 'delete':
#      $record = provision_dns_status('rr', array('zid' => $zone->zid, 'rid' => $args['rid']));
# call the engine, and determine if the zone needs to be removed as well..
#      module_invoke_all('provision_dns_rr', 'delete', $zone, $record);

# TODO:
#      db_query('DELETE FROM {provision_dns_rr} WHERE zid = %d AND rid = %d', $zone->zid, $record->rid);
      break;
  }
}

/**
 * hook_dns_commit: Enable any recent changes/updates to the given DNS server to go live.
 **/
function provision_dns_commit($nid) {
  if ($nid) {
    $dns = node_load($nid);
    $func = 'provision_'.$dns->engine.'_dns_commit';
    call_user_func($func);
  }
}

/**
 * The default template provided for the zonefile configuration
 **/
function _provision_dns_default_template() {
  return file_get_contents(dirname(__FILE__) . "/provision_dns_zonefile.tpl.php");
}


/**
 * Helper function to increment a zone's serial number.
 *
 * @param $serial
 *    A serial in YYYYMMDDnn format
 *
 * @return
 *    The serial, incremented based on current date and index
 **/
function _provision_dns_increment_serial($serial) {
  $date = substr($serial, 0, 8); # Get the YYYYMMDD part
  $index = substr($serial, 8, 2); # Get the index part
  $today = date('Ymd');
  if ($date == $today) {
    return $date . sprintf('%02d', $index+1);
  } else {
    return $today . '01';
  }
}

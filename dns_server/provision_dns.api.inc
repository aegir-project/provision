<?php

// DNS API functions (ie: manage the local DB and drive the DNS engine)

/**
 * Helper function for creating a new zone via the provision_dns_zone API function.
 *
 * @param $domain
 *   The domain name/origin for the zone to be created.
 * @param $data
 *   An array of data to populate the new zone. Most importantly, the 'dns_id' 
 *   field must contain the nid of the DNS server which on which to privision this zone.
 *
 * @return
 *   The zone object, whether newly created, or already existing.
 **/
function provision_dns_create_zone($domain, $data) {
  $writable = provision_path("writable", PROVISION_NAMED_PATH, TRUE, NULL, 
                             t("DNS configuration path @path is not writable"), 
                             PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR);

  if ($zone = provision_dns_status('zone', array('domain' => $domain))) {
    provision_log("Notice", t("Loading zone file for url @url", array('@url' => $domain)));
    return $zone;
  } else {
    provision_log("Notice", t("Generating zone file for url @url on dns server @id", array('@url' => $domain, '@id' => $data['dns_id'])));
    # Setup args array from defaults
    $dns = node_load($data['dns_id']);
    $zone_fields['dns_id'] = $data['dns_id'];
    $zone_fields['origin'] = $domain;
    $zone_fields['serial'] = date('Ymd') . '01';
    $ns_recs = explode("\n", $dns->ns);
#ob_start(); print_r($ns_recs); $out = ob_get_contents(); ob_end_clean();
provision_log("notice", "provision_dns_create_zone ns_recs: ".$ns_recs[0].", ".$ns_recs[1]);
    $zone_fields['ns1'] = $ns_recs[0];
    $zone_fields['ns2'] = $ns_recs[1];
    $zone_fields['mbox'] = $dns->mbox;
    provision_dns_zone('add', $zone_fields);
#    $zone = provision_dns_status('zone', array('domain' => $domain));
#    foreach ($ns_recs as $ns) {
#      provision_dns_create_record('@', $zone, array('type' => 'NS', 'dns_ip' => trim($ns))); 
#    }
    return provision_dns_status('zone', array('domain' => $domain));
  }
}

/**
 * Helper function for creating a new A record via the provision_dns_rr API function.
 *
 * @param $host
 *   The name part of the record to be created.
 * @param $zone
 *   The zone object in which to create the record.
 * @param $data
 *   An array of data used to populate the record. Most importantly, the 'dns_ip' field should
 *   contain the IP address which the host record should point to.
 *
 * @return
 *   The record object, or FALSE if the record could not be created.
 **/
function provision_dns_create_record($host, $zone, $data) {
  provision_log("notice", t("Creating %type record for url @url", array('%type' => $data['type'], '@url' => $host)));
  $rec['name'] = $host;
  $rec['type'] = $data['type']?$data['type']:'A';
  $rec['data'] = $data['dns_ip'];
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
  $url_parts = _provision_dns_split_url($url);

  if ($zone = provision_dns_status('zone', array('domain' => $url_parts['domain']))) {
    if ($rec = provision_dns_status('rr', array('zid' => $zone->zid, 'name' => $url_parts['host'], 'type' => 'A'))) {

      # delete the A record for this site..
      provision_dns_rr('delete', $zone, array('rid' => $rec->rid));

      # and (as long as it's the only host on this domain) delete the zone as well
      if (provision_dns_getrecords($zone->zid, TRUE) <= 1) { 
	provision_dns_zone('delete', array('zid' => $zone->zid));
      }
    }
  }
}


/**
 * Query status of a DNS entry (zone or RR)
 *
 * Get info about a domain's SOA record or a particular RR as an object.
 *
 * @param $op
 *   What kind of DNS entry to return. Valid $ops are 'zone' and 'rr'.
 * @param $args
 *   An array of fields to identify the object to return.  This must always contain 
 *   either a 'zid' or 'domain' key to identify the zone being queried. For RR queries, 
 *   $args must also contain either a 'rid' key OR a 'name' + 'type' key to identify the RR.
 *
 * @return
 *   An object representing the zone or Resource Record, depending on $op.
 **/
function provision_dns_status($op, $args) {

  if (!isset($args['zid']) && isset($args['domain'])) {
    # TODO: error-check this!
    $args['zid'] = _provision_dns_get_zid($args['domain']);
  }

  switch ($op) {
    case 'zone':
      // return zone SOA info if it exists
      $result = db_query("SELECT * FROM {provision_dns_soa} WHERE zid = %d", $args['zid']);
      $object = db_fetch_object($result);
      break;
    case 'rr':
      // return RR details, if it exists
      if (isset($args['rid'])) {
        $result = db_query("SELECT * FROM {provision_dns_rr} WHERE zid = %d AND rid = %d", $args['zid'], $args['rid']);
      } else {
        $result = db_query("SELECT * FROM {provision_dns_rr} WHERE zid = %d AND name = '%s' AND type = '%s'", $args['zid'], $args['name'], $args['type']);
      }
      $object = db_fetch_object($result);	
      break;
  }

  return ($object === false)?FALSE:$object;
}

/**
 * Get all records for a particular zone.
 *
 * @param $zid
 *   The internal zone ID for which records are being requested.
 * @param $num
 *   If TRUE, return only a count of how many records exist in the given zone. 
 *   Otherwise, return an array of the records themselves.
 *
 * @return
 *   An array of record objects, or a count of how many records the zone contains (if $num == TRUE).
 **/
function provision_dns_getrecords($zid, $num = FALSE) {
  $records = array();
  $result = db_query("SELECT * FROM {provision_dns_rr} WHERE zid = %d", $zid);
  while ($rec = db_fetch_object($result)) {
    $records[$rec->rid] = $rec;
  }
  if ($num) { 
    return count($records);
  } else { 
    return $records;
  }
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
      if ($zone = provision_dns_status('zone', array('domain' => $args['origin']))) {
        // domain already exists, just return the zid
        return $zone->zid;
      } else {
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
	$sql = sprintf("INSERT INTO {provision_dns_soa} (%s) VALUES ('%s')",
			implode(",", $f),
			implode("','", $v));
        db_query($sql);
        $zone = provision_dns_status('zone', array('domain' => $args['origin']));
      }
      module_invoke_all("provision_dns_zone", $op, $zone);
      if ($dns) {  # Add a default A record for the origin
        provision_dns_rr('add', $zone, array('name' => '@', 'type' => 'A', 'data' => $dns->default_ip));
      }
      break;
    case 'update':
      if (!isset($args['zid'])) {
        $zone = provision_dns_status('zone', array('domain' => $args['origin']));
      } else {
        $zone = provision_dns_status('zone', array('zid' => $args['zid']));
      }

      if ($zone || provision_dns_status('zone', $args)) {
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
	  $sql = "UPDATE {provision_dns_soa} SET ". implode(',', $keys) ." WHERE zid=%d";
	  db_query($sql, $values);
        }
      } else {
        // zone doesn't exist, return error? 
        return;
      }
      module_invoke_all("provision_dns_zone", $op, $zone);
      return $zone;
      break;
    case 'delete':
      $zone = provision_dns_status('zone', $args);
      module_invoke_all('provision_dns_zone', 'delete', $zone);  # This will delete all the zone's records too

      db_query('DELETE FROM {provision_dns_rr} WHERE zid = %d', $zone->zid);
      db_query('DELETE FROM {provision_dns_soa} WHERE zid = %d', $zone->zid);
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
  if (is_numeric($zone)) {
    $zone = provision_dns_status('zone', array('zid' => $zone));
  } elseif (!is_object($zone)) {
    return FALSE;  # Error, the zone doesn't seem to exist!
  }

  // do whatever DB setup is necessary, then call hook_dns_rr($op, $args)
  switch ($op) {
    case 'add':
      if ($rr = provision_dns_status('rr', array_merge(array('zid' => $zone->zid), $args))) {
	// RR already exists
	return $rr;
      } 
      if (!isset($args['name']) || !isset($args['type']) || !isset($args['data'])) {  # Not enough info
	return FALSE;
      } 
      $sql = sprintf("INSERT INTO {provision_dns_rr} (zid, name, type, data, aux, ttl) VALUES (%d, '%s', '%s', '%s', %d, %d)",
		     $zone->zid, $args['name'], $args['type'], $args['data'], $args['aux'], $args['ttl']);
      db_query($sql);
      $record = provision_dns_status('rr', array('zid' => $zone->zid, 'name' => $args['name'], 'type' => $args['type']));
      module_invoke_all('provision_dns_rr', $op, $zone, $record);
      provision_dns_zone('update', array('zid' => $zone->zid, 'serial' => _provision_dns_increment_serial($zone->serial)));
      return $record;
      break;
    case 'update':
      $record = provision_dns_status('rr', array('zid' => $zone->zid, 'rid' => $args['rid']));
      
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
	$sql = "UPDATE {provision_dns_rr} SET ". implode(',', $keys) ." WHERE zid=%d AND rid=%d";
	db_query($sql, $values);
      }
      module_invoke_all('provision_dns_rr', $op, $zone, $record);
      provision_dns_zone('update', array('zid' => $zone->zid, 'serial' => _provision_dns_increment_serial($zone->serial)));
      return $record;
      break;
    case 'delete':
      $record = provision_dns_status('rr', array('zid' => $zone->zid, 'rid' => $args['rid']));
      module_invoke_all('provision_dns_rr', 'delete', $zone, $record);

      db_query('DELETE FROM {provision_dns_rr} WHERE zid = %d AND rid = %d', $zone->zid, $record->rid);
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


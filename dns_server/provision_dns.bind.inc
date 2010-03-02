<?php
// $Id: provision_dns.bind.inc,v 1.3 2009/03/26 01:40:39 anarcat Exp $

/**
 * @file BIND dns api engine
 *
 * A collection of functions used by the DNS provision hooks to accomplish their tasks.
 * Uses the 'soa' and 'rr' objects defined by the provision_dns.module to manage BIND zonefiles
 */

/**
 * Implementation of hook_dns_zone from provision_dns.api.inc
 *
 * @param $op
 *   The operation to be executed on the zone. Valid operations are 'add', 'update', and 'delete'.
 * @param $zone
 *   The zone object to be operated on (as returned by provision_dns_status).
 **/
function provision_bind_provision_dns_zone($op, $zone) {
  switch ($op) {
    case 'add':
     provision_bind_create_zone_config($zone);
     provision_bind_update_zone($zone);
     break;
   case 'update':
     provision_bind_update_zone($zone, TRUE);
     break;
   case 'delete':
     provision_bind_delete_zone($zone); # And all it's records with it!
     break;
   // do others later
  }
  _provision_bind_restart();
}

/**
 * Implementation of hook_dns_rr from provision_dns.api.inc
 *
 * @param $op
 *   The operation to be executed on the record. Valid operations are 'add', 'update', and 'delete'.
 * @param $zone
 *   The zone object on which to operate (as returned by provision_dns_status)
 * @param $record
 *   The record object on which to operate (as returned by provision_dns_status)
 **/
function provision_bind_provision_dns_rr($op, $zone, $record) {

  switch ($op) {
    case 'add':
      _provision_bind_add_record($zone, $record);
      break;
    case 'update':
      _provision_bind_update_record($zone, $record);
      break;
    case 'delete':
      _provision_bind_delete_record($zone, $record);
      break;
  }
}

/**
 * Create a new entry for the zone provided in the local named.conf file, pointing to the zonefile.
 *
 * @param $zone
 *   The zone object to create an entry for (as returned by provision_dns_status).
 **/
function provision_bind_create_zone_config($zone) {
  // add the zone to our local named.conf file
  $path = variable_get('provision_bind_zonedata', PROVISION_CONFIG_PATH .'/named');
  $conf = PROVISION_CONFIG_PATH . '/' . variable_get('provision_bind_conf', 'named.conf.drupal');

  $zone_str = sprintf('zone "%s" { type master; file "%s/%s"; allow-query { any; }; };%s', $zone->origin, $path, $zone->origin, "\n");

  if ($fd = fopen($conf, "a")) {
    $fd = fwrite($fd, $zone_str);
  }
  if (!$fd) {
    drush_log("Error", dt("error adding the zone to the bind configuration in @path", array("@path" => $conf)));
    drush_set_error(PROVISION_PERM_ERROR);
  }
  @fclose($fd);
}

/**
 * Create/Update a zonefile with SOA and NS records, plus all other records provision_dns knows about
 *
 * @param $zone
 *   The zone object to create or update (as returned by provision_dns_status).
 * @param $exists
 *   If the $exists argument is true, then the function will check for an old_origin field in the zone object,
 *   indicating the name of the zone has changed. If this is the case, the old zonefile will be removed and
 *   the name altered in the named.conf
 *
 * This should use a template, a la:
 * $template = variable_get('provision_dns_zonefile_template', _provision_dns_default_template());
 * But currently, just writes hardcoded string for all of this stuff directly into the zonefile.
 **/
function provision_bind_update_zone($zone, $exists = FALSE) {
  // write a zonefile in the appropriate place
  $path = variable_get('provision_bind_zonedata', '');

  # First setup the SOA record
  $zone_str = sprintf("\$TTL %s\n", $zone->ttl);
  $zone_str .= sprintf("@\t IN\t SOA\t %s. %s. (\n", $zone->origin, str_replace('@', '.', $zone->mbox));
  $zone_str .= sprintf("\t\t\t %s ; Serial\n", $zone->serial);
  $zone_str .= sprintf("\t\t\t %s ; Refresh\n", $zone->refresh);
  $zone_str .= sprintf("\t\t\t %s ; Retry\n", $zone->retry);
  $zone_str .= sprintf("\t\t\t %s ; Expire\n", $zone->expire);
  $zone_str .= sprintf("\t\t\t %s)\t\t; Negative Cache TTL\n\n", $zone->ttl);

  # Next do the NS records
  $zone_str .= sprintf("@\t IN\t NS\t\t %s.\n", $zone->ns1);
  if ($zone->ns2) {
    $zone_str .= sprintf("@\t IN\t NS\t\t %s.\n", $zone->ns2);
  }

  $records = provision_dns_getrecords($zone->zid);
  foreach ($records as $rid => $rec) {
    $zone_str .= _provision_bind_record_string($rec);
  }

  $path = variable_get('provision_bind_zonedata', PROVISION_CONFIG_PATH .'/named');
  $conf = PROVISION_CONFIG_PATH . '/' . variable_get('provision_bind_conf', 'named.conf.drupal');

  if ($exists && isset($zone->old_origin)) { 				  # The name of the zone has changed
#drush_log("notice", "Updating zone ".$zone->old_origin." to ".$zone->origin);
    provision_path("unlink", $path . '/' . $zone->old_origin);            # Remove the old zonefile
    _provision_bind_editfile($conf, '^zone..'.$zone->old_origin.'.*$');   # Remove the old zone from named.conf
    provision_bind_create_zone_config($zone);                             # And re-create the zone in named.conf
  }

#drush_log("notice", "Writing zonefile: ".$zone_str);
  if ($fd = fopen($path . '/'. $zone->origin, "w")) {
    fwrite($fd, $zone_str);  # Write the zonefile
    fclose($fd);
  } else {
    drush_set_error('PROVISION_PERM_ERROR');
  }
}

/**
 * Remove the zonefile and named.conf entry for a given zone
 *
 * @param $zone
 *   The zone object (as returned by provision_dns_status) to remove.
 **/
function provision_bind_delete_zone($zone) {
  # Remove the zone from the named.conf file..
  $path = variable_get('provision_bind_zonedata', PROVISION_CONFIG_PATH .'/named');
  $conf = PROVISION_CONFIG_PATH . '/' . variable_get('provision_bind_conf', 'named.conf.drupal');
  $zone_re = sprintf('^zone..%s.*$', $zone->origin);
  _provision_bind_editfile($conf, $zone_re);

  # and remove the zonefile itself
  $zonefile = $path . '/' . $zone->origin;
  provision_path("unlink", $zonefile);
}

/**
 * hook_dns_commit implementation
 **/
function provision_bind_dns_commit() {
  return _provision_bind_restart();
}

/**
 * Add an RR entry in the given zone's zonefile for the record provided.
 *
 * @param $zone
 *   The zone object (as returned by provision_dns_status) in which to add the RR
 * @param $record
 *   The RR object (as returned by provision_dns_status) to add an entry for.
 **/
function _provision_bind_add_record($zone, $record) {
  $file = variable_get('provision_bind_zonedata', PROVISION_CONFIG_PATH .'/named') . '/' . $zone->origin;
  $line = _provision_bind_record_string($record);

  if ($fh = fopen($file, 'a')) {
    fwrite($fh, $line);
    fclose($fh);
  } else {
    drush_set_error('PROVISION_PERM_ERROR');
  }
}

/**
 * Update an RR entry in the given zone's zonefile for the record provided.
 *
 * @param $zone
 *   The zone object (as returned by provision_dns_status) in which the RR record resides.
 * @param $record
 *   The record object (as returned by provision_dns_status) to update.
 **/
function _provision_bind_update_record($zone, $record) {
  $path = variable_get('provision_bind_zonedata', PROVISION_CONFIG_PATH .'/named');
  $file = $path . '/' . $zone->origin;
  if (!isset($record->old_name)) { $record->old_name = $record->name; }
  $line = _provision_bind_record_string($record);
  _provision_bind_editfile($file, '^'.$record->old_name.'\t.*$', $line);
}

/**
 * Remove an RR entry from the given zone.
 *
 * @param $zone
 *   The zone object (as returned by provision_dns_status) in which the RR record resides.
 * @param $record
 *   The record object (as returned by provision_dns_status) to delete.
 **/
function _provision_bind_delete_record($zone, $record) {
  $path = variable_get('provision_bind_zonedata', PROVISION_CONFIG_PATH .'/named');
  $file = $path . '/' . $zone->origin;
  $line = _provision_bind_record_string($record);
  _provision_bind_editfile($file, '^'.$line.'$');
}

/**
 * Helper function to restart BIND
 **/
function _provision_bind_restart() {
  // restart bind
  $cmd = "sudo ". variable_get('provision_bind_rndc', '/usr/sbin/rndc') . " reload > /dev/null";
  system($cmd, $status);
  return $status;
}

/**
 * Helper function to gather BIND's status (currently unused)
 **/
function _provision_bind_status() {
  $cmd = "sudo ". variable_get('provision_bind_rndc', '/usr/sbin/rndc') . " status > /dev/null";
  system($cmd, $status);
  return $status;
}

/**
 * Helper function to check a given zone's zonefile (currently unused)
 **/
function _provision_bind_namedcheckzone($zone) {
  $path = variable_get('provision_bind_zonedata', '');
  $cmd = variable_get('provision_bind_namedcheckzone', '/usr/sbin/named-checkzone') ." ". $zone->origin ." ". $path . $zone->origin ." > /dev/null";
  system($cmd, $status);
  return $status;
}

/**
 * Helper function to check the local named.conf
 **/
function _provision_bind_namedcheckconf($zone) {
  $conf = variable_get('provision_bind_zonedata', '') . variable_get('provision_bind_conf', '');
  $cmd = variable_get('provision_bind_namedcheckconf', '/usr/sbin/named-checkconf') ." ". $conf ." > /dev/null";
  system($cmd, $status);
  return $status;
}

/**
 * Reads in the given file, locates the 'old' line (regexp), and replaces it with the 'new' one, using preg_replace();
 * If $new is empty, simply removes the 'old' line completely.
 *
 * @param $file
 *   The file to edit.
 * @param $old
 *   A regexp (without delimiters) which locates the line(s) to be edited
 * @param $new
 *   A replacement string for the $old pattern. If this is empty, the $old pattern will be removed completely.
 **/
function _provision_bind_editfile($file, $old, $new = '') {

  # Read in the file, checking each line and doing a preg_replace on matching lines
  if ($fd = fopen($file, 'r')) {
    while ($line = fgets($fd)) {
      if (preg_match("/$old/", $line)) {
	if ($new) {
	  $lines[] = preg_replace("/$old/", $new, trim($line));
	}
      } else {
	$lines[] = $line;
      }
    }
    fclose($fd);
  } else {
    drush_set_error('PROVISION_PERM_ERROR');
  }

  # Write out the changed lines to the original file again.
  if (!file_put_contents($file, $lines)) {
    drush_set_error('PROVISION_PERM_ERROR');
  }
}

/**
 * Helper function to generate a RR line from a record object
 *
 * @param $record
 *  The record object as returned by provision_dns_status
 *
 * @return
 *  A string representing the given record as a line for a zonefile
 **/
function _provision_bind_record_string($record) {
  if ($record->type == "MX") { $pri = $record->aux; } else { $pri = ""; }
  if (( $record->type == "NS" ||
	$record->type == "PTR" ||
	$record->type == "CNAME" ||
	$record->type == "MX" ||
	$record->type == "SRV") && ($record-data != "@")
      ) {
	$destination = $record->data .".";
  } elseif ($record->type == "TXT") {
    $destination = "\"". $record->data ."\"";
  } else {
    $destination = $record->data;
  }

  return $record->name ."\t IN\t ". $record->type ."\t". $pri ."\t". $destination ."\n";
}

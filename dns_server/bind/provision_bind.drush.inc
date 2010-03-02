<?php
// $Id: provision_dns.bind.inc,v 1.3 2009/03/26 01:40:39 anarcat Exp $

/**
 * @file BIND dns api engine
 *
 * A collection of functions used by the DNS provision hooks to accomplish their tasks.
 * Uses the 'soa' and 'rr' objects defined by the provision_dns.module to manage BIND zonefiles
 */

/**
 * Create a new entry for the zone provided in the local named.conf file, pointing to the zonefile.
 *
 * @param $zone
 *   The zone object to create an entry for (as returned by provision_dns_status).
 **/
function provision_bind_create_zone_config($zone) {
  // add the zone to our local named.conf file
  $path = drush_get_option('config_path') . '/named/';
  $conf = drush_get_option('config_path') . '/named.conf.aegir';

  $zone_str = sprintf('zone "%s" { type master; file "%s/%s"; allow-query { any; }; };%s', $zone->origin, $path, $zone->origin, "\n");

  if ($fd = fopen($conf, "a")) {
    $fd = fwrite($fd, $zone_str);
  }
  if (!$fd) {
    drush_set_error(PROVISION_PERM_ERROR, dt("error adding the zone to the bind configuration in @path", array("@path" => $conf)));
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
function provision_bind_update_zone($origin) {
  // write a zonefile in the appropriate place
  $path = drush_get_option('config_path') . '/named/';
  $zone = (object) drush_get_option('zone');
  # First setup the SOA record
  $zone_str = sprintf("\$TTL %s\n", $zone->ttl);
  $zone_str .= sprintf("@\t IN\t SOA\t %s. %s. (\n", $zone->origin, str_replace('@', '.', $zone->mbox));
  $zone_str .= sprintf("\t\t\t %s ; Serial\n", $zone->serial);
  $zone_str .= sprintf("\t\t\t %s ; Refresh\n", $zone->refresh);
  $zone_str .= sprintf("\t\t\t %s ; Retry\n", $zone->retry);
  $zone_str .= sprintf("\t\t\t %s ; Expire\n", $zone->expire);
  $zone_str .= sprintf("\t\t\t %s)\t\t; Negative Cache TTL\n\n", $zone->ttl);

  foreach ($zone->records as $rid => $rec) {
    $zone_str .= _provision_bind_record_string((object)$rec);
  }

  $conf = drush_get_option('config_path') . '/named.conf.aegir';

  if (file_exists($path . $zone->old_origin)) {
#drush_log("notice", "Updating zone ".$zone->old_origin." to ".$zone->origin);
    provision_path("unlink", $path . '/' . $zone->old_origin);            # Remove the old zonefile
  }
  if (isset($zone->old_origin)) { 				  # The name of the zone has changed
    _provision_bind_editfile($conf, '^zone..'.$zone->old_origin.'.*$');   # Remove the old zone from named.conf
  }
  if (!file_exists($path . $zone->origin)) {
    provision_bind_create_zone_config($zone);                             # And re-create the zone in named.conf
  }

#drush_log("notice", "Writing zonefile: ".$zone_str);
  drush_log("zone: " . var_export($zone, true));
  $zonefile = $path . '/'. $zone->origin;
  drush_log("zonefile: $zonefile");
  if ($fd = fopen($zonefile, "w")) {
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
  $cmd = drush_get_option('provision_bind_rndc', 'sudo /usr/sbin/rndc') . " reload";
  system($cmd, $status);
  return ($status == 0);
}

/**
 * Helper function to gather BIND's status (currently unused)
 **/
function _provision_bind_status() {
  $cmd = "sudo ". variable_get('provision_bind_rndc', '/usr/sbin/rndc') . " status > /dev/null";
  system($cmd, $status);
  return ($status == 0);
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

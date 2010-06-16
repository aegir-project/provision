<?php

/**
 * Create a new entry for the zone provided in the local named.conf file, pointing to the zonefile.
 *
 * @param $zone
 *   The zone object to create an entry for (as returned by dns_status).
 **/
function bind_create_zone_config($zone) {
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

function bind_delete_zone_config($origin) {
  $path = drush_get_option('config_path') . '/named/';
  $conf = drush_get_option('config_path') . '/named.conf.aegir';
  $zone_re = sprintf('^zone..%s.*$', $zone->origin);
  _bind_editline($conf, $zone_re);

}

/**
 * Create a zonefile with SOA header plus all other records
 *
 * @param $zone
 *   The zone object to create or update
 *
 * This should use a template, a la:
 * $template = variable_get('dns_zonefile_template', _dns_default_template());
 * But currently, just writes hardcoded string for all of this stuff directly into the zonefile.
 **/
function bind_create_zone($zone) {
  // write a zonefile in the appropriate place
  $path = drush_get_option('config_path') . '/named/';
  # First setup the SOA record
  $zone_str = sprintf("\$TTL %s\n", $zone->ttl);
  $zone_str .= sprintf("@\t IN\t SOA\t %s. %s. (\n", $zone->origin, str_replace('@', '.', $zone->mbox));
  $zone_str .= sprintf("\t\t\t %s ; Serial\n", $zone->serial);
  $zone_str .= sprintf("\t\t\t %s ; Refresh\n", $zone->refresh);
  $zone_str .= sprintf("\t\t\t %s ; Retry\n", $zone->retry);
  $zone_str .= sprintf("\t\t\t %s ; Expire\n", $zone->expire);
  $zone_str .= sprintf("\t\t\t %s)\t\t; Negative Cache TTL\n\n", $zone->ttl);

  foreach ($zone->records as $rid => $rec) {
    $zone_str .= _bind_record_string((object)$rec);
  }

  bind_create_zone_config($zone);                             # And re-create the zone in named.conf

  drush_log("notice", dt("Creating zonefile @zonefile", array('@zonefile' => $zone->origin)));
  $zonefile = $path . '/'. $zone->origin;
  if ($fd = fopen($zonefile, "w")) {
    $status = fwrite($fd, $zone_str);  # Write the zonefile
    fclose($fd);
  } else {
    $status = drush_set_error('PROVISION_PERM_ERROR', dt("Cannot write zonefile @path", array("@path" => $zonefile)));
  }
  return $status;
}

/**
 * Remove the zonefile and named.conf entry for a given zone
 *
 * @param $zone
 *   The zone object (as returned by dns_status) to remove.
 **/
function bind_delete_zone($zone) {
  # Remove the zone from the named.conf file..
  $path = drush_get_option('config_path') . '/named/';

  # and remove the zonefile itself
  $zonefile = $path . '/' . $zone->origin;
  if ($status = provision_path("unlink", $zonefile)) {
    $status = bind_delete_zone_config($zone->origin);
  }
  return $status;
}

/**
 * Add an RR entry in the given zone's zonefile for the record provided.
 *
 * @param $zone
 *   The zone object (as returned by dns_status) in which to add the RR
 * @param $record
 *   The RR object (as returned by dns_status) to add an entry for.
 **/
function _bind_add_record($zone, $record) {
  $file = drush_get_option('config_path') . '/named/' . $zone->origin;
  $line = _bind_record_string($record);

  if ($fh = fopen($file, 'a')) {
    $status = fwrite($fh, $line);
    fclose($fh);
  } else {
    $status = drush_set_error('PROVISION_PERM_ERROR', dt("Cannot write record to file @path", array("@path" => $file)));
  }
  return $status;
}

/**
 * Update an RR entry in the given zone's zonefile for the record provided.
 *
 * @param $zone
 *   The zone object (as returned by dns_status) in which the RR record resides.
 * @param $record
 *   The record object (as returned by dns_status) to update.
 **/
function _bind_update_record($zone, $record) {
  $file = drush_get_option('config_path') . '/named/' . $zone->origin;
  if (!isset($record->old_name)) { $record->old_name = $record->name; }
  $line = _bind_record_string($record);
  _bind_editline($file, '^'.$record->old_name.'\t.*$', $line);
}

/**
 * Remove an RR entry from the given zone.
 *
 * @param $zone
 *   The zone object (as returned by dns_status) in which the RR record resides.
 * @param $record
 *   The record object (as returned by dns_status) to delete.
 **/
function _bind_delete_record($zone, $record) {
  $file = drush_get_option('config_path') . '/named/' . $zone->origin;
  $line = _bind_record_string($record);
  _bind_editline($file, '^'.$line.'$');
}

/**
 * Helper function to restart BIND
 **/
function _bind_restart() {
  return _bind_reload();
}

function _bind_reload($zone = "") {
  // restart bind
  $cmd = drush_get_option('bind_rndc', 'sudo /usr/sbin/rndc') . " reload " . $zone;
  system($cmd, $status);
  return ($status == 0);
}

/**
 * Helper function to gather BIND's status (currently unused)
 **/
function _bind_status() {
  $cmd = "sudo ". drush_get_option('bind_rndc', '/usr/sbin/rndc') . " status > /dev/null";
  system($cmd, $status);
  return ($status == 0);
}

/**
 * Helper function to check a given zone's zonefile (currently unused)
 **/
function _bind_namedcheckzone($zone) {
  $file = drush_get_option('config_path') . '/named/' . $zone->origin;
  $cmd = drush_get_option('bind_namedcheckzone', '/usr/sbin/named-checkzone') ." ". $zone->origin ." ". $path . $zone->origin ." > /dev/null";
  system($cmd, $status);
  return $status;
}

/**
 * Helper function to check the local named.conf
 **/
function _bind_namedcheckconf($zone) {
  $conf = drush_get_option('config_path') . '/named.conf.aegir';
  $cmd = drush_get_option('bind_namedcheckconf', '/usr/sbin/named-checkconf') ." ". $conf ." > /dev/null";
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
function _bind_editline($file, $old, $new = '') {

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
 *  The record object as returned by dns_status
 *
 * @return
 *  A string representing the given record as a line for a zonefile
 **/
function _bind_record_string($record) {
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

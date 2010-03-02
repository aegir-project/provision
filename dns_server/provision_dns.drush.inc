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

/**
 * Implementation of hok_drush_command().
 */
function provision_dns_drush_command() {
  $items['provision zone'] = array(
    'arguments' => array('operation' => dt('The operation to perform on a zone (verify, delete, modify)')),
    'description' => dt('Manipulate a zonefile'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  return $items;
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

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

//include_once(dirname(__FILE__) . '/../provision.service.inc');

/**
 * Register our directory as a place to find provision classes.
 */
function dns_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Implements hook_drush_init().
 */
function dns_drush_init() {
  dns_provision_register_autoload();
}

/**
 * Implementation of hook_drush_command().
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
    $record = d()->service('dns')->config('zone', $zone)->record_get($name);
    if (!is_array($record[$type])) {
      $record[$type] = array();
    }
    $record[$type] = array_merge($record[$type], array($destination));
    $status = d()->service('dns')->config('zone', $zone)->record_set($name, $record)->write();
    break;
  case 'rr-modify':
    $status = d()->service('dns')->config('zone', $zone)->record_set($name, array($type => array($destination)))->write();
    break;
  case 'rr-delete':
    if ($type) {
      $status = d()->service('dns')->config('zone', $zone)->record_set($name, array($type => NULL))->write();
    } else {
      $status = d()->service('dns')->config('zone', $zone)->record_set($name, NULL)->write();
    }
    break;
  default:
    $status = drush_set_error('DRUSH_WRONG_ARGUMENT', dt("wrong argument provided to provision-zone"));
    break;
  }
  // XXX: we shouldn't reload zones unless we have actually done changes, but return values of the above functions are always FALSE
  d()->service('dns')->parse_configs();
  
  return $status;
}

/**
 *  Implements hook_provision_services().
 */
function dns_provision_services() {
  dns_provision_register_autoload();
  return array('dns' => NULL);
}

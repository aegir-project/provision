<?php

/**
 * @file
 *    Mysql provisioning module.
 *
 * The goal of this module is to create mysql databases and user accounts, for sites that are about to be created.
 * It uses the provision API to tie into the right places in the site creation work flow.
 */

//include_once(dirname(__FILE__) . '/../provision.service.inc');

/**
 * Implements hook_provision_services().
 */
function db_provision_services() {
  db_provision_register_autoload();
  return array('db' => NULL);
}

/**
 * Implements hook_drush_exit().
 */
function db_drush_exit() {
  // determine how to close it too.
  d()->service('db')->close();
}

/**
 * Implements hook_drush_help().
 */
function db_drush_help($section) {
  switch ($section) {
    case 'error:PROVISION_CONNECT_DB_FAILED' :
      return dt('Unable to connect to database server.');
    case 'error:PROVISION_CREATE_DB_FAILED' :
      return dt('Unable to create new databases.');
    case 'error:PROVISION_GRANT_DB_USER_FAILED' :
      return dt('Unable to grant privileges to database users.');
    case 'error:PROVISION_DROP_DB_FAILED' :
      return dt('Unable to drop database.');
  }
}

/**
 * Implements hook_drush_init().
 */
function db_drush_init() {
  db_provision_register_autoload();
}

/**
 * Register our directory as a place to find provision classes.
 */
function db_provision_register_autoload() {
  static $loaded = FALSE;
  if (!$loaded) {
    $loaded = TRUE;
    provision_autoload_register_prefix('Provision_', dirname(__FILE__));
  }
}

/**
 * Indicates the place holders that should be replaced in _db_query_callback().
 */
define('PROVISION_QUERY_REGEXP', '/(%d|%s|%%|%f|%b)/');

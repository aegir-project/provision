<?php
// $Id$
/**
 * @file mysql db api extension
 *
 * A collection of helper functions used by the main provision hooks to accomplish their tasks.
 */

function _provision_db_connection($conn = NULL) {
  static $connection = NULL;
  if (!is_null($conn)) {
    $connection = $conn;
  }
  return $connection;
}

function provision_db_connect() {
  if (_provision_db_connection()) {
    return TRUE;
  }
  $connection = @mysql_connect(drush_get_option('master_db_host'), drush_get_option('master_db_user'), drush_get_option('master_db_passwd'));

  if (!$connection) {
    drush_set_error('PROVISION_MASTER_DB_FAILED', dt('Could not connect to the master database server (%err).', array('%err' => mysql_error())), 'error');
  }
  else {
    $success = @mysql_select_db('mysql', $connection);
    if ($success) {
      _provision_db_connection($connection);
    }
    else {
      drush_set_error('PROVISION_MASTER_DB_FAILED', dt('Could not select the mysql database (%err).', array('%err' => mysql_error())), 'error');
      return FALSE;
    }
  }
  return TRUE;
}


function provision_db_query($query) {
  $args = func_get_args();
  array_shift($args);
  if (isset($args[0]) and is_array($args[0])) { // 'All arguments in one array' syntax
    $args = $args[0];
  }
  _provision_db_query_callback($args, TRUE);
  $query = preg_replace_callback(PROVISION_QUERY_REGEXP, '_provision_db_query_callback', $query);
  return _provision_db_query($query);
}

/**
 * Helper function for db_query().
 */
function _provision_db_query($query, $debug = 0) {
  $result = mysql_query($query, _provision_db_connection());

  if (!mysql_errno(_provision_db_connection())) {
    return $result;
  }
  else {
    drush_log( mysql_error(_provision_db_connection()) ."\nquery: ". $query, 'error');
    return FALSE;
  }
}

function provision_db_result($result, $row = 0) {
  if ($result && mysql_num_rows($result) > $row) {
    return mysql_result($result, $row);
  }
  return FALSE;
}

/**
 * Fetch one row from the result set
 *
 * @see db_fetch_array
 */
function provision_db_fetch_array($result) {
  if ($result) {
    return mysql_fetch_array($result, MYSQL_ASSOC);
  }
  return $result;
}

/**
 * Indicates the place holders that should be replaced in _db_query_callback().
 */
define('PROVISION_QUERY_REGEXP', '/(%d|%s|%%|%f|%b)/');


function _provision_db_query_callback($match, $init = FALSE) {
  static $args = NULL;
  if ($init) {
    $args = $match;
    return;
  }

  switch ($match[1]) {
    case '%d': // We must use type casting to int to convert FALSE/NULL/(TRUE?)
      return (int) array_shift($args); // We don't need db_escape_string as numbers are db-safe
    case '%s':
      return mysql_real_escape_string(array_shift($args), _provision_db_connection());
    case '%%':
      return '%';
    case '%f':
      return (float) array_shift($args);
    case '%b': // binary data
      return "'" . mysql_real_escape_string(array_shift($args), _provision_db_connection()) . "'";
  }
}

function provision_db_close() {
  if (_provision_db_connection()) {
    $conn = _provision_db_connection();
    mysql_close($conn);
  }
}

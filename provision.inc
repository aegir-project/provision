<?php
// $Id$

/**
 * @file
 * The provisioning framework API.
 *
 * API functions that are used by the provisioning framework to provide structure to the provisioning modules.
 *
 * @see errorhandling
 * @see logging
 * @see sitedata
 * @see provisionvalues
 */

drush_errors_on();

/**
 * @defgroup sitedata Site data management utility functions.
 * @{
 * The provision framework maintains a site.php file in the sites directory, to maintain additional
 * information from the front end, as well as providing a change history of setting changes. 
 *
 * These functions load, save and manage changes made to the site data. This data has diagnostic and infrastructure
 * values, that allow sites to be more easily moved between different provisioned platforms.
 */

/**
 * Make a determination whether or not the given
 * host is local or not.
 *
 * We needed to fork this from drush core to handle the case sensitivity in host names.
 *
 * @param host
 *   A hostname, 'localhost' or '127.0.0.1'.
 * @return
 *   True if the host is local.
 */
function provision_is_local_host($host) {
  $host = strtolower($host);
  // In order for this to work right, you must use 'localhost' or '127.0.0.1'
  // or the machine returned by 'uname -n' for your 'remote-host' entry in
  // your site alias.  Note that sometimes 'uname -n' does not return the
  // correct value.  To fix it, put the correct hostname in /etc/hostname
  // and then run 'hostname -F /etc/hostname'.
  return ($host == 'localhost') || 
    ($host == '127.0.0.1') ||
    (gethostbyname($host) == '127.0.0.1') ||
    (gethostbyname($host) == '127.0.1.1') || // common setting on
                                             // ubuntu and friends
    ($host == strtolower(php_uname('n'))) ||
    ($host == provision_fqdn());
}

/**
 * return the FQDN of the machine or provided host
 *
 * this replicates hostname -f, which is not portable
 */
function provision_fqdn($host = null) {
  if (is_null($host)) {
    $host = php_uname('n');
  }
  return strtolower(gethostbyaddr(gethostbyname($host)));
}

/**
 * Retrieve a base_url for the currently active site.
 *
 * TODO: when we actually support HTTPS, do this correctly.
 */
function provision_get_base_url() {
  $base_url = 'http://' . d()->uri;

  $http_port = d()->server->http_port;
  if (!is_null($http_port) && ($http_port != 80)) {
    $base_url .= ':' . $http_port;
  }
  return $base_url;
}

/**
 * Save modified options to the drushrc.php file
 */
function provision_save_server_data() {
  if (!drush_get_error()) {
    $config = new provisionConfig_drushrc_server(d()->name);
    $config->write();
  }
}


function provision_save_site_data() {
  if (!drush_get_error()) {
    $config = new provisionConfig_drushrc_site(d()->name);
    $config->write();
    provision_drupal_sync_site();
  }
}


/**
 * Save modified options to the drushrc.php file
 */
function provision_save_platform_data() {
  if (!drush_get_error()) {
    $config = new provisionConfig_drushrc_platform(d()->name);
    $config->write();
    provision_drupal_sync_site();
  }
}


/**
 * @} End of "defgroup sitedata".
 */

/**
 * Remove files or directories, recursively
 *
 * This was taken from Drupal 7's file.inc, with slight modifications:
 *  * carry error codes along the way (returns TRUE only if all operations return TRUE)
 *  * remove any type of files encountered (not files and directories)
 *  * do not follow symlink directories
 *
 * @see file_unmanaged_delete_recursive()
 */
function _provision_recursive_delete($path) {
  $ret = 1;
  // is_dir() follows symlinks, so it can return true on a symlink
  if (is_dir($path) && !is_link($path)) {
    $d = dir($path);
    while (($entry = $d->read()) !== FALSE) {
      if ($entry == '.' || $entry == '..') {
        continue;
      }
      $entry_path = $path .'/'. $entry;
      $ret &= _provision_recursive_delete($entry_path);
    }
    $d->close();

    $rm = provision_file()->rmdir($path)
      ->succeed('Deleting @path directory successful.')
      ->fail('Deleting @path directory failed.')
      ->status();

    $ret = $ret && $rm;
  }
  else {
    $rm = provision_file()->unlink($path)
      ->fail('Deleting @path file failed.')
      ->status();
    $ret = $ret && $rm;
  }
  return $ret;
}

/**
 * Convenience copy of Drupal 6's file_check_location()
 *
 * Check if a file is really located inside $directory. Should be used to make
 * sure a file specified is really located within the directory to prevent
 * exploits.
 *
 * @code
 *   // Returns FALSE:
 *   file_check_location('/www/example.com/files/../../../etc/passwd', '/www/example.com/files');
 * @endcode
 *
 * @param $source A string set to the file to check.
 * @param $directory A string where the file should be located.
 * @return 0 for invalid path or the real path of the source.
 *
 * @see file_check_location()
 */
function _provision_file_check_location($source, $directory = '') {
  $check = realpath($source);
  if ($check) {
    $source = $check;
  }
  else {
    // This file does not yet exist
    $source = realpath(dirname($source)) .'/'. basename($source);
  }
  $directory = realpath($directory);
  if ($directory && strpos($source, $directory) !== 0) {
    return 0;
  }
  return $source;
}


/**
 * Find the username of the current running procses
 *
 * This will return the username of the current running user (as seen
 * from posix_geteuid()) and should be used instead of
 * get_current_user() (which looks at the file owner instead).
 *
 * @see get_current_user()
 * @see posix_geteuid()
 *
 * @return
 *   String. The username.
 */
function provision_current_user() {
  return provision_posix_username(posix_geteuid());
}

/**
 * Check whether a user is a member of a group.
 *
 * @param user
 *   username or user id of user.
 * @param group
 *   groupname or group id of group.
 *
 * @return
 *   Boolean. True if user does belong to group, 
 *   and FALSE if the user does not belong to the group, or either the user or group do not exist.
 */
function provision_user_in_group($user, $group) {
  // TODO: make these singletons with static variables for caching.
  $user = provision_posix_username($user);
  $group = provision_posix_groupname($group);
  if ($user && $group) {
    $info = posix_getgrnam($group);
    if (in_array($user, $info['members'])) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Return the valid system username for $user.
 *
 * @return
 *   Returns the username if found, otherwise returns FALSE
 */
function provision_posix_username($user) {
  // TODO: make these singletons with static variables for caching.
  // we do this both ways, so that the function returns NULL if no such user was found.
  if (is_numeric($user)) {
    $info = posix_getpwuid($user);
    $user = $info['name'];
  }
  else {
    $info = posix_getpwnam($user);
    $user = $info['name'];
  }
  return $user;
}

/**
 * Return the valid system groupname for $group.
 *
 * @return
 *   Returns the groupname if found, otherwise returns FALSE
 */
function provision_posix_groupname($group) {
  // TODO: make these singletons with static variables for caching.
  // we do this both ways, so that the function returns NULL if no such user was found.
  if (is_numeric($group)) {
    $info = posix_getgrgid($group);
    $group = $info['name'];
  }
  else {
    $info = posix_getgrnam($group);
    $group = $info['name'];
  }
  return $group;
}

/**
 * Generate a random alphanumeric password.
 *
 * This is a copy of Drupal core's user_password() function. We keep it
 * here in case we need this and don't have a bootstrapped Drupal
 * around.
 *
 * @see user_password()
 */
function provision_password($length = 10) {
  // This variable contains the list of allowable characters for the
  // password. Note that the number 0 and the letter 'O' have been
  // removed to avoid confusion between the two. The same is true
  // of 'I', 1, and 'l'.
  $allowable_characters = 'abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789';

  // Zero-based count of characters in the allowable list:
  $len = strlen($allowable_characters) - 1;

  // Declare the password as a blank string.
  $pass = '';

  // Loop the number of times specified by $length.
  for ($i = 0; $i < $length; $i++) {

    // Each iteration, pick a random character from the
    // allowable string and append it to the password:
    $pass .= $allowable_characters[mt_rand(0, $len)];
  }

  return $pass;
}

/**
 * This is a helper function which changes deeply nested objects into arrays
 *
 * This helps get past the face that objects are not simple to work with, or
 * save in context files.
 *
 * This function 'misuses' a side effect of the json_decode function's second
 * parameter. As this is done in C, and the structures we are manipulating
 * aren't that large, it should be performant enough.
 */
function _scrub_object($input) {
  return json_decode(json_encode($input), TRUE);
}

/**
 * Execute a command against a specific context object.
 *
 * @arg string $target
 *   the context to operate on, @ prefix is optional
 * @arg string $command
 *   drush command passed to drush_backend_invoke_args()
 * @arg string $arguments
 *   drush arguments passed to drush_backend_invoke_args()
 * @arg string $data
 *   drush data passed to drush_backend_invoke_args()
 * @arg string $mode
 *   drush IPC mode (GET/POST) passed to drush_backend_invoke_args()
 *
 * @see drush_backend_invoke_args()
 */
function provision_backend_invoke($target, $command, $arguments = array(), $data = array(), $mode = 'GET') {
  return drush_backend_invoke_args('@' . ltrim($target, '@') . ' ' . $command, $arguments, $data + array('root' => null, 'uri' => null), $mode);
}

/**
 * the aegir version of the backend
 *
 * @return string
 *  the aegir version as stored in the .info file, potentially
 *  including the 6.x- prefix. to get a cleaned up version, use
 *  provision_version_parts()
 *
 * @see provision_version_parts()
 */
function provision_version() {
  $ini = parse_ini_file(dirname(__FILE__) . '/provision.info');
  return $ini['version'];
}

/**
 * Aegir API implemented by this backend
 *
 * This is the major release number, the first part of the version
 * stored in the info file
 *
 * @return int 
 *   a number greater than zero, 1 for 1.0 or 1.0-rc2, 2 for 2.0, etc.
 *
 * @see provision_version_parts()
 */
function provision_api_version() {
  $parts = provision_version_parts();
  return $parts[0];
}

/**
 * The different parts of the version number
 *
 * This cleans up the version number by removing the Drupal version
 * (6.x-...) and splits the remaining version on dots.
 *
 * @return array 
 *   the major and minor version numbers, e.g. array(1, 0-rc3) for
 *   1.0-rc3 or array(1, 2) for 1.2
 */
function provision_version_parts() {
  $version = preg_replace('/^[^-]*-/', '', provision_version()); // remove "6.x-"
  return explode('.', $version);
}

/**
 * A base class for the service and file handling classes that implements
 * chaining of methods.
 */
class provisionChainedState {
  protected $last_status;
  protected $tokens;

  /**
   * Clear internal state
   */
  protected function _clear_state() {
    $this->last_status = NULL;
    $this->tokens = NULL;
  }

  /**
   * Return the status of the last operation.
   *
   * @return
   *   TRUE or FALSE for success or failure; NULL if there was not a previous
   *   operation.
   */
  function status() {
    return $this->last_status;
  }

  /**
   * Log a notice into the logging system, if the last operation completed
   * succesfully.
   * 
   * @param $message
   *   The message to log, a string.
   */
  function succeed($message) {
    if ($this->last_status === TRUE) {
      drush_log(dt($message, $this->tokens), 'message');      
    }

    return $this;
  }

  /**
   * Log a notice into the logging system, if the last operation did not
   * complete succesfully.
   * 
   * @param $message
   *   Log this as a error to the logging system, if the $error_codes parameter
   *   has been set, otherwise, log this as a warning. If the operation
   *   specifies an additional reason for the operation failing, it will be
   *   appended to this message.
   *
   * @param error_codes
   *   Generate these system level errors using the provision error bitmasks.
   */
  function fail($message, $error_codes = NULL) {
    if (!empty($this->tokens['@reason'])) {
      $message .= ' (@reason)';
    }
    if ($this->last_status === FALSE) {
      if (is_null($error_codes)) {
        // Trigger a warning
        drush_log(dt($message, $this->tokens), 'warning');
      }
      else {
        // Trigger a sysem halting error
        drush_set_error($error_codes, dt($message, $this->tokens));
      }
    }

    return $this;
  }
}

// Base class for provision exceptions.
class provisionException extends Exception {

}

/**
 * Signal for parent to continue processing.
 *
 * The primary use for this class is for the config
 * classes to be able to signal to it's caller, that
 * the configuration file was not needed, and to
 * continue on.
 */
class provisionException_continue extends provisionException {

}

/**
 * Provision class.
 *
 * This is just a container for some useful static methods.
 */
class provision {
  /**
   * The actual body of the method_invoke function.
   * 
   * This is a static method so it can be re-used by some other classes
   * that aren't contexts. (notably services and configs).
   */
  static function method_invoke($object, $func, $args = array()) {
    if (method_exists($object, $func)) {
      return call_user_func_array(array($object, $func), $args);
    }
  }
}



include_once('provision.context.inc');
include_once('provision.context.server.inc');
include_once('provision.context.platform.inc');
include_once('provision.context.site.inc');
include_once('provision.service.inc');
include_once('provision.file.inc');
include_once('provision.config.inc');



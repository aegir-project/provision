<?php
// $Id$
include_once('provision.environment.inc');
include_once('provision.service.inc');

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
  }
}


/**
 * Save modified options to the drushrc.php file
 */
function provision_save_platform_data() {
  if (!drush_get_error()) {
    $config = new provisionConfig_drushrc_platform(d()->name);
    $config->write();
  }
}


/**
 * @} End of "defgroup sitedata".
 */

/**
 * Remove files or directories, recursively
 *
 * This was taken from imagecache.module, with slight modifications:
 * - carry error codes along the way (returns TRUE only if all operations return TRUE)
 * - remove any type of files encountered (not just links, files and dirs)
 * - safety checking since we don't necessarly trust the removed files
 */
function _provision_recursive_delete($path) {
  $ret = 1;
  if (is_dir($path)) {
    $d = dir($path);
    while (($entry = $d->read()) !== FALSE) {
      if ($entry == '.' || $entry == '..') continue;
      $entry_path = $path .'/'. $entry;
      if (_provision_file_check_location($entry_path, $path)) {
        $ret = _provision_recursive_delete($entry_path);
      } 
      else {
        $ret = 0;
      }
    }

    $rm = provision_service('file')->rmdir($path)
      ->succeed('Deleting @path directory successful.')
      ->fail('Deleting @path directory failed.')
      ->status();

    $ret = $ret && $rm;
  }
  else {
    $rm = provision_service('file')->unlink($path)
      ->fail('Deleting @path file failed.')
      ->status();
    $ret = $ret && $rm;
  }
  return $ret;
}

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
 * Wrapper around drush_shell_exec to provide sprintf functionality with some more safety.
 *
 * @TODO: fix this so we can get error codes and the return values. drush_shell_exec is too
 *        limited
 */
function provision_shell_exec() {
  $args = func_get_args();
  
  //do not change the command itself, just the parameters.
  for ($x = 1; $x < sizeof($args); $x++) {
    $args[$x] = escapeshellcmd($args[$x]);
  }
  $command = call_user_func_array('sprintf', $args);
  drush_log($command); 
  return drush_shell_exec($command);
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

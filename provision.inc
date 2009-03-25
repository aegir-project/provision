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

define('PROVISION_OUTPUT', 'PROVISION_OUTPUT_START>>>%s<<<PROVISION_OUTPUT_END');

/**
 * Invoke provision api calls. 
 * 
 * Call the correct hook for all the modules that implement it. We can not use Drupal's default module_invoke, because we
 * can not pass references through it.
 * Additionally, the ability to rollback when an error has been encountered is also provided.
 * If at any point during execution, the drush_get_error() function returns anything but 0, provision_invoke will
 * trigger $hook_rollback for each of the hooks that implement it, in reverse order from how they were executed.
 *
 * This function will also trigger pre_$hook and post_$hook variants of the hook and it's rollbacks automatically.
 *
 * @param hook
 *   The hook name to be executed for all the modules.
 * @param url
 *   The url of the site being invoked.
 * @param data
 *   A reference to the associated array containing the data for the site. This needs to be a reference, 
 *   because the modules might provide additional information about the site.
 * @return
 *   A boolean specifying whether or not the command was successfully completed. 
 * 
 */
function provision_invoke($hook, $arg1 = NULL, $arg2 = NULL, $arg3 = NULL, $arg4 = NULL, $arg5 = NULL) {
  $files = drush_scan_directory(dirname(__FILE__), $hook . ".provision.inc$");
  if (is_array($files)) {
    foreach ($files as $filename => $info) {
      include_once($filename);
    }
  }

  // First we build a list of functions are about to execute
  $variations = array($hook . "_validate", "pre_$hook", $hook, "post_$hook");
  foreach ($variations as $var_hook) {
    foreach (drush_command_implements("provision_$var_hook") as $name) {
      $functions[] = $name . '_provision_' . $var_hook;
    }
  }

  $rollback = FALSE;
  $completed = array();
  foreach ($functions as $func) {
    $completed[] = $func;
    if (function_exists($func)) {
      $func($arg1, $arg2, $arg3, $arg4, $arg5);
      //this is run before and after so that messages appear in the correct order.
      _drush_log_drupal_messages();
      if (drush_get_error()) {
        drush_log(dt('An error occurred at function : @func', array('@func' => $func)), 'error');
        # As soon as an error occurs, roll back
        $rollback = TRUE;
        break;
      }
    }
  }

  // something went wrong, we need to undo 
  if ($rollback) {
    foreach (array_reverse($completed) as $func) {
      $rb_func = $func . '_rollback';
      if (function_exists($rb_func)) {
        $rb_func($arg1, $arg2, $arg3, $arg4, $arg5);
        _drush_log_drupal_messages();
        drush_log("Changes for $func module have been rolled back.", 'Rollback');
      }
    }
  }

  return !$rollback;
}

function provision_command($hook, $arg1 = NULL, $arg2 = NULL, $arg3 = NULL, $arg4 = NULL, $arg5 = NULL) {
  foreach (drush_command_implements("provision_init") as $name) {
    $func = $name . '_provision_init';
    $func($arg1, $arg2, $arg3, $arg4, $arg5);
    drush_log("Initializing $name");
    _drush_log_drupal_messages();
  }
  if (!drush_get_error()) {
    provision_invoke($hook, $arg1, $arg2, $arg3, $arg4, $arg5);
  }
  if (!drush_get_error()) {
    foreach (drush_command_implements('provision_finalize') as $name) {
      $func = $name . '_provision_finalize';
      $func($arg1, $arg2, $arg3, $arg4, $arg5);
      _drush_log_drupal_messages();
    }
  }
}


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
 * Returns the aggregated site data from both the pre-existing site.php file, and the options passed to Drush
 *
 * This function merges the data from the command line parser, and the information already saved by previous invokations
 * of the api. This provides a single view of all data relating to the site.
 * This function also provides sensible defaults for some of the settings.
 *
 * @param url
 *   The url of the site being invoked.
 * @return
 *   An associated array containing the relevant settings for the site.
 */
function provision_get_site_data($url) {
  if ($old_data = provision_load_site_data($url)) {
    //Merge previously saved data with the new data. This way, new parameters overwrite old ones.
    $site_data = array_merge($old_data, $site_data);    
  }
  
  if ($site_data['aliases'] && !is_array($site_data['aliases'])) {
    $site_data['aliases'] = explode(",", $site_data['aliases']);
  }  
  return $site_data;
}

/**
 * Load site data stored in the site.php file for the specified site.
 *
 * @param url
 *   The url of the site being invoked
 * @return
 *   If the file was found, an associative array of the data that was loaded. Otherwise returns FALSE.
 */
function provision_load_site_data($url) {
  //Load the configuration data.

  //@TODO remove this, it is a temporary measure until all site.php files are placed by drushrc.php files

  $conf_file = "sites/$url/site.php";
  if (file_exists($conf_file)) {
    provision_path('chmod', $conf_file, 0400);
    require($conf_file);

    // The provision 0.1 config data might contain older data,
    // so any data in the new drushrc.php file would be newer.
    $site_context = drush_get_context('site');
    $site_context = array_merge($data, $site_context);
    drush_set_context('site', $site_context);
  }
  return FALSE;
}

/**
 * Save modified options to the site.php file
 *
 * @param url
 *   The url of the site being invoked
 * @param data
 *   The complete data structure that has been created. Only settings that have been changed will be recorded.
 */
function provision_save_site_data() {
  drush_save_config('site');
}

/**
 * @} End of "defgroup sitedata".
 */


/**
 * Save the options for the platform to the drushrc.php in the root of the platform. 
 */
function provision_save_platform_data() {
  drush_save_config('drupal');
}

/**
 * @defgroup provisionvalues Value replacement support for the provisioning framework
 * @{
 */

/**
 * Generate the text for a config file using php
 */
function provision_render_config($template, $variables) {
  _provision_errors_off();
  extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
  ob_start();                      // Start output buffering
  eval('?>'. $template);                 // Generate content
  $contents = ob_get_contents();   // Get the contents of the buffer
  ob_end_clean();                  // End buffering and discard
  _provision_errors_on();
  return $contents;                // Return the contents
}

/**
 * @} End of "defgroup provisionvalues".
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

    $rm = provision_path('rmdir', $path, TRUE,
      dt('Deleting @path directory sucessful.', array('@path' => $path)),
      dt('Deleting @path directory failed.', array('@path' => $path)));

    $ret = $ret && $rm;
  }
  else {
    $rm = provision_path('unlink', $path, TRUE, NULL, 
      dt('Deleting @path file failed.', array('@path' => $path)));
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
  if (is_numeric($user)) {
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

function provision_platform_include($path, $command, $version = null, $platform = 'drupal') {
  $version = ($version) ? $version : drush_drupal_major_version();
  $options[] = sprintf("%s_%s_%s", $platform, $version, $command);
  $options[] = sprintf("%s_%s", $platform, $command);
  $options[] = sprintf("%s", $command);

  $match = false;
  foreach ($options as $option) {
    $file = sprintf("%s/%s.inc", $path, $option);
    if (file_exists($file)) {
      $match = $file;
      break;
    }
  }

  if ($match) {
    drush_log(dt('Including platform specific file : @file', array('@file' => $match)));
    include_once($file);
  }
}

function _provision_errors_on() {
  ini_set('error_reporting', E_ALL ^ E_NOTICE);
  ini_set('display_errors', TRUE);
}
_provision_errors_on();


function _provision_errors_off() {
  ini_set('error_reporting', ~E_ALL);
  ini_set('display_errors', FALSE);
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

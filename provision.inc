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

/**
 * Invoke provision api calls. 
 * 
 * Call the correct hook for all the modules that implement it. We can not use Drupal's default module_invoke, because we
 * can not pass references through it.
 * Additionally, the ability to rollback when an error has been encountered is also provided.
 * If at any point during execution, the provision_get_error() function returns anything but 0, provision_invoke will
 * trigger $hook_rollback for each of the hooks that implement it, in reverse order from how they were executed.
 *
 * @param hook
 *   The hook name to be executed for all the modules.
 * @param url
 *   The url of the site being invoked.
 * @param data
 *   A reference to the associated array containing the data for the site. This needs to be a reference, 
 *   because the modules might provide additional information about the site.
 * @param rollback
 *   A boolean specifying whether or not the entire task needs to be rolled back. 
 *   This is used specifically in commands which implement multiple hooks, such as 'install',
 *   which implements 'pre_install', 'install' and 'post_install' hooks.
 * @return
 *   A boolean specifying whether or not any rollback has been performed. 
 * 
 */
function provision_invoke($hook, $url, &$data, $rollback = FALSE) {
   
  if (!$rollback) {
    foreach (module_implements("provision_$hook") as $name) {
      _provision_log_messages();
      $completed[] = $name;
      $func = $name .'_provision_'. $hook;
      $func($url, $data);
      //this is run before and after so that messages appear in the correct order.
      _provision_log_messages();
      if (provision_get_error()) {
        # As soon as an error occurs, roll back
        $rollback = TRUE;
        break;
      }
    }
  }
  else {
    $completed = module_implements("provision_$hook");
  }
  
  if ($rollback) {
    foreach (array_reverse($completed) as $name) {
      $func = $name .'_'. $hook .'_rollback';
      if (function_exists($func)) {
        $func($url, $data);
        provision_set_log('Rollback', "Changes for $name module have been rolled back.");
      }
    }
    return TRUE;
  }
  else {
    return FALSE;
  }
}


/**
 * Turn drupal_set_message errors into provision_log errors
 */
function _provision_log_messages() {
  $messages = drupal_get_messages();

  //Drupal message errors.
  foreach ((array) $messages['error'] as $error) {
    if (preg_match('/^warning:/', $error)) {
      provision_log('warning', ereg_replace('/^warning: /', '', $error));  
    }
    elseif (preg_match('/^user warning:/', $error)) {
      provision_log("error", ereg_replace('/^user warning: /', '', $error));  
    }
    else {
      provision_log('error', $error);
    }
  }

}



/**
 * Return output to the command line.
 *
 * Provides support for the -b/--backend flag to drush, which returns a serialized data structure.
 * This feature is used for communication with the front end.
 *
 * @param url
 *   The url of the site being invoked.
 * @param data
 *    The complete associative array containing all the aggregated site settings.
 * @param extra
 *   An associative array containing additional data to be returned from the command. @see provision_stats_stats()
 */
function provision_output($url = NULL, $data = array(), $extra = NULL) {
  $return = $extra;

  $return['site'] = $data;
  $error = provision_get_error();
  if (!$error) {
    $error = PROVISION_SUCCESS; // return 1 on success.
  }
  $return['error_status'] = $error; // error code being returned
  $return['log'] = provision_get_log(); // Append logging information
  $return['messages'] = drupal_get_messages();
  if (PROVISION_DRUSH_BACKEND) {
    print serialize($return);
  }
  else {
    if ($return) {
      if ($output = theme('provision_'. $data['task_type'] .'_output', $url, $return)) {
        return $output;
      } 
      else {
        /** TODO : return a cleanly formatted display of all the necessary information */ 
        print_r($return);
      }
    }
  }
  exit($error);
}


/**
 * @defgroup errorhandling Managing errors that occur in the provisioning framework.
 * @{
 * Functions that manage the current error status of the provisioning framework.
 *
 * These functions operate by maintaining a static variable that is a bitmask of all the errors that have occurred.
 * This bitmask value is returned at the end of program execution, and provide the hosting front end more information
 * on how to diagnose any problems that may have occurred.
 */


if (PROVISION_SUCCESS != -1) {
  include_once('provision_errors.inc');
}

/**
 * Set an error code for the error handling system.
 *
 * @param error_code
 *   Any of the defined error status definitions. A numerical bitmask value.
 * @return
 *   The current aggregate error status
 */
function provision_set_error($error_code = 0) {
  static $error = 0;

  if ($error_code) {
    $error = $error | (int) $error_code;    
  }

  return $error;
}

/**
 * Return the current error handling status
 *
 * @return
 *   The current aggregate error status
 */
function provision_get_error() {
  return provision_set_error();
}

/**
 * Check if a specific error status has been set.
 *
 * @param error
 *   Any of the defined error status definitions. A numerical bitmask value. 
 * @return
 *   TRUE if the specified error has been set, FALSE if not 
 */
function provision_cmp_error($error) {
  return provision_get_error() ^ $error;
}

/**
 * @} End of "defgroup errorhandling".
 */

/**
 * @defgroup logging Logging information to be provided as output.
 * @{
 * Functions that allow the provisioning framework to log messages to be provided to the front end.
 *
 * These functions are primarily for diagnostic purposes, but also provide an overview of tasks that were taken
 * by the framework during creation of a site.
 */

/**
 * Maintain a static array containing all the log messages
 *
 * @param entry
 *   Associative array containing the log message.
 * @return
 *   Entire log history, only if $entry is NULL
 */
function _provision_set_log($entry = NULL) {
  static $log = array();
  if ($entry == NULL) {
    return $log;
  }
  else {
    $log[] = $entry;
  }
}

/**
 * Add a log message to the log history.
 *
 * @param type
 *   The type of message to be logged. Common types are 'warning', 'error' and 'notice'.
 * @param message
 *   String containing the message to be logged.
 */
function provision_log($type, $message) {
  _provision_set_log(array(
     'type' => $type, 
     'message' => $message, 
     'timestamp' => time()
    ));
}

/**
 * Retrieve the log messages from the log history
 *
 * @return
 *   Entire log history
 */
function provision_get_log() {
  return _provision_set_log();
}

/**
 * @} End of "defgroup errorhandling".
 */

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
  global $args;
  //TODO: Accept serialized string via unix pipe.
  $site_data = $args['options'];
  $site_data['site_url'] = $url;
  $site_data['task_type'] = $args['commands'][1];
  $site_data['task_id'] = drush_get_option('task_id', NULL);
  $site_data['publish_path'] = PROVISION_DOCROOT_PATH;
  $site_data['profile'] = ($site_data['profile']) ? $site_data['profile'] : variable_get('provision_default_profile', 'default');
  $site_data['site_ip'] =  variable_get('provision_apache_server_ip', '127.0.0.1');
  $site_data['site_port'] =  variable_get('provision_apache_server_ip', 80);
    
  //Default to english language
  $site_data['language'] = $site_data['language'] ? $site_data['language'] : 'en';

  if ($old_data = provision_load_site_data($url)) {
    //Merge previously saved data with the new data. This way, new parameters overwrite old ones.
    $site_data = array_merge($old_data, $site_data);    
  }
  
  if ($site_data['aliases']) {
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
  $conf_file = "sites/$url/site.php";
  if (file_exists($conf_file) && is_readable($conf_file)) {
    require($conf_file);
    return (array) $data;
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
function provision_save_site_data($url, $data) {
  global $args;
  $conf_file = "sites/$url/site.php";
    
  $old_data = provision_load_site_data($url);

  //initialize the file. this is lame, i know. but it will work.
  if (!file_exists($conf_file)) {
    $fp = fopen($conf_file, 'a+'); //Append to the end of the config file.
    fwrite($fp, "<?php\n");
    fclose($fp);
    provision_path("chmod", $conf_file, 0600,
      t("Changed permissions of <code>@path</code> to @confirm"),
      t("Could not change permissions <code>@path</code> to @confirm"),
      PROVISION_PERM_ERROR | PROVISION_INSTALL_ERROR );
  } 

  $fp = fopen($conf_file, "a+");
  if (!$fp) {
    provision_log('error', 'Site config file could not be written');
    provision_set_error(PROVISION_PERM_ERROR);
  }
  else {
    $timestamp = mktime();
    $aid = $data['task_id'];
    fwrite($fp, "\n\n#". format_date($timestamp, 'large'));
    if ($aid) {
      $task = array('task' => $data['task_type'], 'status' => provision_get_error());
      $line = "\n\$tasks[$aid][$timestamp] = ". str_replace(array('  ', "\n"), '', var_export($task, TRUE)) .';';
      fwrite($fp, $line);
    }
    foreach ($data as $key => $value) {
      if ($data[$key] != $old_data[$key]) {
        $line = "\n\$data['$key'] = ". var_export($value, TRUE) .';';
        fwrite($fp, $line);  
      }
    }
    fclose($fp);
  }
}

/**
 * @} End of "defgroup sitedata".
 */


/**
 * Save the options for the platform to the drushrc.php in the root of the platform. 
 */
function provision_save_platform_data() {
  global $args;
  $exclude = array('task_id');
  $conf_file = PROVISION_DOCROOT_PATH . '/drushrc.php';
  $fp = fopen($conf_file, "w");
  if (!$fp) {
    provision_log('error', 'Platform config file could not be written');
    provision_set_error(PROVISION_PERM_ERROR);
  }
  else {
    fwrite($fp, "<?php\n");
    $timestamp = mktime();
    fwrite($fp, "#". format_date($timestamp, 'large'));
    foreach ($args['options'] as $key => $value) {
      if (!in_array($key, $exclude)) {
        $line = "\n\$options['$key'] = ". var_export($value, TRUE) .';';
        fwrite($fp, $line);  
      }
    }
    fwrite($fp, "\n");
    fclose($fp);
  }
}

/**
 * @defgroup provisionvalues Value replacement support for the provisioning framework
 * @{
 */



/**
 * List of values available for the config files
 *
 * @return
 *   A keyed array listing the substitution values.
 */
function provision_value_list() {
    /** TODO: Complete the value list to allow the front end to more easily edit the settings. */
  $values['site_url'] = t('The domain name used to access the site. This is defaulted to the value used on the command line.');
  $values['db_type'] = t('The type of database server used');
  $values['db_username'] = t('Username to access database for site');
  $values['db_password'] = t('Password to access database for site');
  $values['db_name'] = t('Database name for the site');
  $values['profile'] = t('Install profile of site');
  $values['task_type'] = t('What type of task has been used. Only used in conjuction with hosting front end');

  return $values;
}

/**
 * Generate the text for a config file using php
 */
function provision_render_config($template, $variables) {
  extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
  ob_start();                      // Start output buffering
  eval('?>'. $template);                 // Generate content
  $contents = ob_get_contents();   // Get the contents of the buffer
  ob_end_clean();                  // End buffering and discard
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
      if (file_check_location($entry_path, $path)) {
        $ret = _provision_recursive_delete($entry_path);
      } 
      else {
        $ret = 0;
      }
    }

    $rm = provision_path('rmdir', $path, TRUE,
      t('Deleting @path directory sucessful.', array('@path' => $path)),
      t('Deleting @path directory failed.', array('@path' => $path)));

    $ret = $ret && $rm;
  }
  else {
    $rm = provision_path('unlink', $path, TRUE, NULL, 
      t('Deleting @path file failed.', array('@path' => $path)));
    $ret = $ret && $rm;
  }
  return $ret;
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
 
  return drush_shell_exec($command);
}

/**
 * Set the active database.
 *
 * Wrapper around db_set_active, which provides switching out of db_url.
 * @param new_db_url
 *    The database url to set the connection to. If not provided, will switch back to Drupal default.
 */
function provision_set_active_db($new_db_url = NULL) {
  static $old_db_url = NULL;
  global $db_url;
  
  #initialize static
  if (!$old_db_url) {
    $old_db_url = $db_url;
    $db_url = array();
    $db_url['default'] = $old_db_url;
  }

  if ($new_db_url) {
    preg_match("$^([a-z]*)://$", $new_db_url, $matches);

    if ($matches[1] != PROVISION_DB_TYPE) {
      $new_db_url = preg_replace("/^" .$matches[1]. "/", PROVISION_DB_TYPE, $new_db_url);
    }
    $db_url[md5($new_db_url)] = $new_db_url;
    db_set_active(md5($new_db_url));
  }
  else {
    db_set_active('default');
  }
}

/**
 * Close an active connection
 *
 * This is an INCREDIBLY dangerous function, that is only used during import of sites
 * to avoid creating connections for every site being imported
 */
function provision_close_active_db() {
  global $active_db;
  $close_db = $active_db;
  db_set_active('default');
  if ($close_db !== $active_db) {
    db_close($close_db);
  }
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

/**
 * Run an external provision script and integrate it's output
 */
function provision_exec($cmd, &$data) {
  exec($cmd, $return, $code);
  $return = join("", $return);
  provision_log('command', t('Running: @cmd', array('@cmd' => $cmd)));
  $values = unserialize($return);
  if (is_array($values)) {
    $data = array_merge($data, $values['site']);
    foreach ($values['log'] as $log) {
      provision_log($log['type'], $log['message']);
    }
    if (!($code & PROVISION_SUCCESS)) {
      provision_set_error($code);
    }
    return TRUE; 
  }
  else {
    provision_set_error(PROVISION_FRAMEWORK_ERROR);
    provision_log('error',t("The command could not be executed succesfully (returned: !return, code: %code)", array("!return" => $return, "%code" =>  $code)));
    return FALSE;
  }
};

/**
 * A wrapper for drush_get_option for when it is not present
 */
function provision_get_option($option, $default) {
  if (function_exists('drush_get_option')) {
    return call_user_func('drush_get_option', $option, $default);
  }
  else {
    return $default;
  }
}

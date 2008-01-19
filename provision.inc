<?php
/**
 * @file
 * The provisioning framework API.
 *
 * API functions that are used by the provisioning framework to provide structure to the provisioning modules.
 *
 * @see errorhandling
 * @see logging
 * @see sitedata
 * @see provisiontokens
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
 *   A boolean specifying whether or not the entire action needs to be rolled back. 
 *   This is used specifically in commands which implement multiple hooks, such as 'install',
 *   which implements 'pre_install', 'install' and 'post_install' hooks.
 * @return
 *   A boolean specifying whether or not any rollback has been performed. 
 * 
 */
function provision_invoke($hook, $url, &$data, $rollback = false) {
  
  if (!$rollback) {
    foreach (module_implements("provision_$hook") as $name) {
      $completed[] = $name;
      $func = $name . "_provision_" . $hook;
      $func($url, $data);
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
      $func = $name . "_" . $hook . '_rollback';
      if (function_exists($func)) {
        $func($url, $data);
        provision_set_log("Rollback", "Changes for $name module have been rolled back.");
      }
    }
    return FALSE;
  }
  else {
    return TRUE;
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
function provision_output($url, $data, $extra = null) {
  $return = $extra;
  $return['site'] = $data;
  $return['error_status'] = provision_get_error(); // error code being returned
  $return['log'] = provision_get_log(); // Append logging information
  $return['messages'] = drupal_get_messages();
  if (drush_get_option(array('b', 'backend'), FALSE)) {
    print serialize($return);
  }
  else {
    foreach (provision_get_log() as $entry) {
      printf("%10s|%10s|%10%s", $entry['timestamp'], $entry['type'], $entry['severity'], $entry['message']);
    }
    if ($data) {
      /** TODO : return a cleanly formatted display of all the necessary information */ 
      print_r($return);
    }
  }
  exit(provision_get_error());
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
 * These functions are primarily for diagnostic purposes, but also provide an overview of actions that were taken
 * by the framework during creation of a site.
 */

/**
 * Maintain a static array containing all the log messages
 *
 * @param entry
 *   Associative array containing the log message.
 * @return
 *   Entire log history, only if $entry is null
 */
function _provision_set_log($entry = null) {
  static $log = array();
  if ($entry == null) {
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
  #TODO: Accept serialized string via unix pipe.
  foreach ($args['options'] as $key => $value) {
    if (preg_match("/^site-/", $key)) {
      $site_data[$key] = $value;
    }
  }
  $site_data['site-url'] = $url;
  $site_data['site-action-type'] = $args['commands'][1];
  $docroot = variable_get('provision_root', '');
  $site_data['site-document-root'] =  ($docroot) ? $docroot . '/webroot' : $_SERVER['DOCUMENT_ROOT'];
  $site_data['site-temporary-url'] = str_replace('.', '-', $url) . '.' . variable_get('provision_tempurl_base', $_SERVER['HTTP_HOST']);
  $site_data['site-profile'] = ($site_data['site-profile']) ? $site_data['site-profile'] : variable_get('provision_default_profile', 'default');
  $site_data['site-ip'] =  variable_get('provision_apache_server_ip', '127.0.0.1');
  $site_data['site-port'] =  variable_get('provision_apache_server_ip', 80);
  if ($old_data = provision_load_site_data($url)) {
    # Merge previously saved data with the new data. This way, old parameters overwrite new ones.
    $site_data = array_merge(provision_load_site_data($url), $site_data);    
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
  # Load the configuration data.
  $conf_file = "sites/$url/site.php";
  if (file_exists($conf_file) && is_readable($conf_file)) {
    require_once($conf_file);
    return (array) $data;
  }
  return false;
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
    
  $old_data = provision_load_site_data($url);
  
  $conf_file = "sites/$url/site.php";
  $fp = fopen($conf_file, "w+"); # Append to the end of the config file.
  if (!$fp) {
    provision_log("error", "Site config file could not be written");
    provision_set_error(PROVISION_PERM_ERROR);
  }
  else {
    fwrite($fp, "<?php\n");
    $action = array('id' => $data['action-id'], 
                    'timestamp' => mktime(), 
                    'action' => $data['site-action-type'], 
                    'success' => $data['success']);
    $line = "\n\n\$actions[] = " . var_export($action, TRUE) . ';';
    fwrite($fp, $line);

    foreach ($data as $key => $value) {
      if ($data[$key] != $old_data[$key]) {
        $line = "\n\$data['$key'] = " . var_export($value, true) . ';';
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
 * @defgroup provisiontokens Token replacement support for the provisioning framework
 * @{
 * The provisioning framwork makes use of the token module for value replacement in configuration file templates.
 */


/**
 * Implementation of hook_token_values(). 
 *
 * @param type
 *   A flag indicating the class of substitution tokens to return
 *   information on. 
 * @return
 *   A keyed array listing the substitution tokens. Elements should be
 *   in the form of: $list[$type][$token] = $description
 */
function provision_token_values($type, $object = null) {
  global $args;
  switch ($type) {
    case 'site':
      $values = array_merge(provision_get_site_data($args['commands'][2]), $object) ;
      break;
  }
  return (array) $values;  
  
}
/**
 * Implementation of hook_token_list(). 
 *
 * @param type
 *   A flag indicating the class of substitution tokens to return
 *   information on. 
 * @return
 *   A keyed array listing the substitution tokens. Elements should be
 *   in the form of: $list[$type][$token] = $description
 */
function provision_token_list($type = 'all') {
  if ($type == 'site') {
    /** TODO: Complete the token list to allow the front end to more easily edit the settings. */
    $tokens['site']['site-url'] = t("The domain name used to access the site.");
    $tokens['site']['site-db-type'] = t("The type of database server used");
    $tokens['site']['site-db-username'] = t("Username to access database for site");
    $tokens['site']['site-db-password'] = t("Password to access database for site");
    $tokens['site']['site-db-name'] = t("Database name for the site");
    $tokens['site']['site-profile'] = t("Installation Profile");
    $tokens['site']['site-action-type'] = t("What type of action has been used");
  }
  return $tokens;
}

/**
 * @} End of "defgroup provisiontokens".
 */

/**
 * Confirm that provision is running through Drush.
 *
 * This module requires a 'bootstrap' and 'configtest' feature, which is to be used during installation, to make sure that
 * all the provisioning modules have the correct permissions / access to be able to do their jobs. This bootstrap process should
 * be used when deploying new platforms (ie: new releases).
 */

function _provision_confirm_drush() {
  #confirm that code is running through the command line.
}

/**
 * Get the root path of the Provision installation
 */
function _provision_root_path() {
  return variable_get('provision_root', ereg_replace("/webroot$", "", $_SERVER['DOCUMENT_ROOT']));
}

/**
 * Wrapper around drush_shell_exec to provide sprintf functionality with some more safety.
 */
function provision_shell_exec() {
  $args = func_get_args();
  
  #do not change the command itself, just the parameters.
  for ($x = 1; $x < sizeof($args); $x++) {
    $args[$x] = escapeshellcmd($args[$x]);
  }
  $command = call_user_func_array("sprintf", $args);
 
  return drush_shell_exec($command);
}
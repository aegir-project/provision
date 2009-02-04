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
 * If at any point during execution, the provision_get_error() function returns anything but 0, provision_invoke will
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
function provision_invoke($hook, &$data, $arg1 = NULL, $arg2 = NULL, $arg3 = NULL, $arg4 = NULL, $arg5 = NULL) {
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
      $func($data, $arg1, $arg2, $arg3, $arg4, $arg5);
      //this is run before and after so that messages appear in the correct order.
      _provision_log_messages();
      if (provision_get_error()) {
        provision_log('error', pt('An error occurred at function : @func', array('@func' => $func)));
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
        $rb_func($data, $arg1, $arg2, $arg3, $arg4, $arg5);
        _provision_log_messages();
        provision_log('Rollback', "Changes for $func module have been rolled back.");
      }
    }
  }

  return !$rollback;
}

function provision_command($hook, $arg1 = NULL, $arg2 = NULL, $arg3 = NULL, $arg4 = NULL, $arg5 = NULL) {
  $data = array();
  foreach (drush_command_implements("provision_init") as $name) {
    $func = $name . '_provision_init';
    $func($data, $arg1, $arg2, $arg3, $arg4, $arg5);
    provision_log('notice', "Initializing $name");
    _provision_log_messages();
  }
  if (!provision_get_error()) {
    $success = provision_invoke($hook, $data, $arg1, $arg2, $arg3, $arg4, $arg5);
    if ($success) {
      foreach (drush_command_implements('provision_finalize') as $name) {
        $func = $name . '_provision_finalize';
        $func($data, $arg1, $arg2, $arg3, $arg4, $arg5);
        _provision_log_messages();
      }
    }
  }
  provision_output($data);
}


/**
 * Turn drupal_set_message errors into provision_log errors
 */
function _provision_log_messages() {
  if (function_exists('drupal_get_messages')) {

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
}

function provision_parse_output($string) {
  $regex = sprintf(PROVISION_OUTPUT, '(.*)');

  preg_match("/$regex/s", $string, $match);
  if ($match[1]) {
    // we have our serialized string
    $output = $match[1];
    // remove the match we just made and any non printing characters
    $string = trim(str_replace(sprintf(PROVISION_OUTPUT, $match[1]), '', $string));
    if ($string) {
      $messages[] = array('type' => 'warning', 'message' => $string, 'timestamp' => time());
    }
  }

  if ($output) {
    $data = unserialize($output);
    if (is_array($data)) {
      if (is_array($messages)) {
        $data['log'] = array_merge($data['log'], $messages);
      }
      return $data;
    }
  }
  return false;
}



/**
 * Return output to the command line.
 *
 * Provides support for the -b/--backend flag to drush, which returns a serialized data structure.
 * This feature is used for communication with the front end.
 *
 * @param data
 *    The complete associative array containing all the aggregated site settings.
 * @param extra
 *   An associative array containing additional data to be returned from the command. @see provision_stats_stats()
 */
function provision_output($data = array()) {
  $return['site'] = $data;
  $error = provision_get_error();
  if (!$error) {
    $error = PROVISION_SUCCESS; // return 1 on success.
  }
  $return['error_status'] = $error; // error code being returned
  $return['log'] = provision_get_log(); // Append logging information
  if (PROVISION_DRUSH_BACKEND) {
    printf(PROVISION_OUTPUT, serialize($return));
  }
  else {
    if ($return) {
      /** TODO : return a cleanly formatted display of all the necessary information */ 
      print_r($return);
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
  $site_data['profile'] = ($site_data['profile']) ? $site_data['profile'] : 'default';
  $site_data['site_ip'] =  ($site_data['site_ip']) ? $site_data['site_ip'] : '127.0.0.1';
  $site_data['site_port'] =  ($site_data['site_port']) ? $site_data['site_port'] : '80';
    
  //Default to english language
  $site_data['language'] = $site_data['language'] ? $site_data['language'] : 'en';

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
  $conf_file = "sites/$url/site.php";
  if (file_exists($conf_file)) {
    provision_path('chmod', $conf_file, 0400);
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
function provision_save_site_data($data, $url) {
  global $args;
  $conf_file = "sites/$url/site.php";

  $exclude = array('task_id', 'url', 'backend');
  $options = array();
  include(PROVISION_DOCROOT_PATH . '/drushrc.php'); // load drush rc
  $exclude = array_merge($exclude, array_keys($options));
  if (file_exists($conf_file) && !is_writable($conf_file)) {
    provision_path("chmod", $conf_file, 0600, dt("Made site.php file writable"));
  }
  //initialize the file. this is lame, i know. but it will work.
  $fp = fopen($conf_file, 'w'); //Append to the end of the config file.
  if (!$fp) {
    provision_log('error', 'Site config file could not be written');
    provision_set_error(PROVISION_PERM_ERROR);
  }
  else {
    fwrite($fp, "<?php\n");
    $timestamp = mktime();
#    fwrite($fp, "#". format_date($timestamp, 'large'));
    foreach ($data as $key => $value) {
      if (!in_array($key, $exclude)) {
        $line = "\n\$data['$key'] = ". var_export($value, TRUE) .';';
        fwrite($fp, $line);  
      }
    }
    fwrite($fp, "\n");
    fclose($fp);
    provision_path('chmod', $conf_file, 0400, dt('Protected site.php file'));
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
#    fwrite($fp, "#". format_date($timestamp, 'large'));
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
  $values['site_url'] = dt('The domain name used to access the site. This is defaulted to the value used on the command line.');
  $values['db_type'] = dt('The type of database server used');
  $values['db_username'] = dt('Username to access database for site');
  $values['db_password'] = dt('Password to access database for site');
  $values['db_name'] = dt('Database name for the site');
  $values['profile'] = dt('Install profile of site');
  $values['task_type'] = dt('What type of task has been used. Only used in conjuction with hosting front end');

  return $values;
}

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

function provision_proc_open($cmd, &$data = NULL) {
  $descriptorspec = array(
     0 => array("pipe", "r"),  // stdin is a pipe that the child will read from
     1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
     2 => array("file", "/tmp/error-output.txt", "a") // stderr is a file to write to
  );

  $process = proc_open($cmd, $descriptorspec, $pipes, null, null, array('context' => $context));
  if (is_resource($process)) {
    if ($data) {
      fwrite($pipes[0], serialize($data)); // pass the data array in a serialized string
    }
    fclose($pipes[0]);
    $info = stream_get_meta_data($pipes[1]);
    stream_set_blocking($pipes[0], TRUE);
    stream_set_timeout($pipes[1], 1);
    $stream = '';
    while (!feof($pipes[1]) && !$info['timed_out']) { 
      $string .= fgets($pipes[1], 4096);
      $info = stream_get_meta_data($pipes[1]);
      ob_flush();
      flush();
    };
    fclose($pipes[1]);
    $code = proc_close($process);
    return array('cmd' => $cmd, 'output' => $string, 'code' => $code);
  }
  return false;
}


/**
 * Run an external provision script and integrate it's output
 */
function provision_exec($cmd, &$data) {
  provision_log('command', dt('Running: @cmd', array('@cmd' => $cmd)));
  $proc = provision_proc_open($cmd, $data);
  if ($proc['output']) {
    $values = provision_parse_output($proc['output']);
    if (is_array($values)) {
      $data = array_merge($data, $values['site']);
      foreach ($values['log'] as $log) {
        provision_log($log['type'], $log['message']);
      }
      if (!($proc['code'] & PROVISION_SUCCESS)) {
        provision_set_error($proc['code']);
      }
      return TRUE; 
    }
    else {
      provision_set_error(PROVISION_FRAMEWORK_ERROR);
      provision_log('error', dt("The command could not be executed succesfully (returned: !return, code: %code)", array("!return" => $proc['output'], "%code" =>  $proc['code'])));
      return FALSE;
    }
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

/**
 * A wrapper for dt(). The st() function is too specific for our purposes
 */
function pt($string, $args = array()) {
  if (function_exists('t')) {
    return call_user_func('t', $string, $args);
  }
  else {
    // Transform arguments before inserting them
    foreach ($args as $key => $value) {
      switch ($key[0]) {
        // Escaped only
        case '@':
          $args[$key] = htmlspecialchars($value, ENT_QUOTES);
          break;
        // Escaped and placeholder
        case '%':
        default:
          $args[$key] = '<em>'. htmlspecialchars($value, ENT_QUOTES) .'</em>';
          break;
        // Pass-through
        case '!':
      }
    }
    return strtr($string, $args);
  }
}

/**
 * This is a script that allows external scripts to initialize themselves to a certain url (usually $argv[1])
 *
 * Some scripts need to run with drupal as the only active database, and this allows us to do that.
 */
function provision_external_init($url, $bootstrap = TRUE) {
  $url = ($url) ? $url : 'default';
  $drupal_base_url = parse_url(sprintf("http://" . $url));
  $_SERVER['HTTP_HOST'] = $drupal_base_url['host'];
  $_SERVER['PHP_SELF'] = $drupal_base_url['path'].'/install.php';
  $_SERVER['REQUEST_URI'] = $_SERVER['SCRIPT_NAME'] = $_SERVER['PHP_SELF'];
  $_SERVER['REMOTE_ADDR'] = '';
  $_SERVER['REQUEST_METHOD'] = NULL;
  $_SERVER['SERVER_SOFTWARE'] = NULL;

  define('DRUPAL_ROOT', realpath('.'));
  define('PROVISION_DRUSH_BACKEND', true);
  if ($bootstrap) {
    include_once('includes/bootstrap.inc');
    drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
  }
  ob_start();
  print stream_get_contents(STDIN);
  $string = ob_get_contents();
  ob_end_clean();
  if (trim($string)) {
    return unserialize($string);
  }
  return TRUE;
}

function provision_internal_init($url, $bootstrap = TRUE) {
  $url = ($url) ? $url : 'default';
  $drupal_base_url = parse_url(sprintf("http://" . $url));
  $_SERVER['HTTP_HOST'] = $drupal_base_url['host'];
  $_SERVER['PHP_SELF'] = '/index.php';
  $_SERVER['REQUEST_URI'] = $_SERVER['SCRIPT_NAME'] = $_SERVER['PHP_SELF'];
  $_SERVER['REMOTE_ADDR'] = '127.0.0.1';
  $_SERVER['REQUEST_METHOD'] = NULL;
  $_SERVER['SERVER_SOFTWARE'] = NULL;
  if ($bootstrap) {
    include_once('./includes/bootstrap.inc');
    drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
  }

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

function provision_platform_include(&$data, $path, $command, $version = null, $platform = 'drupal') {
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
    provision_log('notice', dt('Including platform specific file : @file', array('@file' => $match)));
    include_once($file);
  }
}

function _provision_errors_on() {
#  ini_set('error_reporting', E_ALL);
#  ini_set('display_errors', TRUE);
}
_provision_errors_on();


function _provision_errors_off() {
  ini_set('error_reporting', ~E_ALL);
  ini_set('display_errors', FALSE);
}

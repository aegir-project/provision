<?php
/**
 * @file
 * The provisioning framework API.
 *
 * API functions that are used by the provisioning framework to provide
 * structure to the provisioning modules.
 */

drush_errors_on();

/**
 * Return an instance of the provision autoloader.
 *
 * This will instiatate an instance if it needs to.
 */
function provision_autoload() {
  static $instance = NULL;

  if (is_null($instance)) {
    require_once(dirname(__FILE__) . '/Symfony/Component/ClassLoader/UniversalClassLoader.php');
    $instance = new UniversalClassLoader();
    // Activate the autoloader.
    $instance->register();
  }

  return $instance;
}

/**
 * Register a PECL style prefix with the provision autoloader.
 *
 * @param $prefix
 *   The class prefix to register.
 * @param $dir
 *   The directory to search for the classes in.
 * @param $prepend
 *   If the directory should be searched first for the classes in the given
 *   prefix, set this to TRUE, otherwise, the default, FALSE, is fine.
 */
function provision_autoload_register_prefix($prefix, $dir, $prepend = FALSE) {

  // Get any current directories set for this prefix.
  $current_prefixes = provision_autoload()->getPrefixes();
  if (isset($current_prefixes[$prefix])) {
    $dirs = $current_prefixes[$prefix];
  }
  else {
    $dirs = array();
  }

  // Now add the new one.
  if ($prepend) {
    array_unshift($dirs, $dir);
  }
  else {
    array_push($dirs, $dir);
  }

  // Set the prefixes.
  provision_autoload()->registerPrefix($prefix, $dirs);
}

// Add our prefix to the autoloader.
provision_autoload_register_prefix('Provision_', dirname(__FILE__));

/**
 * Return the directory containing the file a class is defined in.
 *
 * @param $class_name
 *   The class name to search for.
 * @return
 *   A directory if the class can be found or an empty string if not.
 */
function provision_class_directory($class_name) {
  return dirname(provision_class_file($class_name));
}

/**
 * Return the file a class is defined in.
 *
 * @param $class_name
 *   The class name to search for.
 * @return
 *   A file if the class can be found or an empty string if not.
 */
function provision_class_file($class_name) {
  if (class_exists($class_name)) {
    $reflect = new reflectionClass($class_name);
    return $reflect->getFilename();
  }
  return '';
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
function provision_fqdn($host = NULL) {
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
    $config = new Provision_Config_Drushrc_Server(d()->name);
    $config->write();
  }
}


function provision_save_site_data() {
  if (!drush_get_error()) {
    $config = new Provision_Config_Drushrc_Site(d()->name);
    $config->write();
    provision_drupal_sync_site();
  }
}


/**
 * Save modified options to the drushrc.php file
 */
function provision_save_platform_data() {
  if (!drush_get_error()) {
    $config = new Provision_Config_Drushrc_Platform(d()->name);
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
    while ($dir && ($entry = $d->read()) !== FALSE) {
      if ($entry == '.' || $entry == '..') {
        continue;
      }
      $entry_path = $path . '/' . $entry;
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
 * @param $target
 *   the context to operate on, @ prefix is optional.
 * @param $command
 *   drush command passed to drush_invoke_process().
 * @param $arguments
 *   drush arguments passed to drush_invoke_process().
 * @param $data
 *   drush data passed to drush_invoke_process().
 * @param $mode
 *   drush IPC mode (GET/POST) passed to drush_invoke_process().
 *
 * @see drush_invoke_process()
 */
function provision_backend_invoke($target, $command, $arguments = array(), $data = array(), $mode = 'GET') {
  $context = '@' . ltrim($target, '@');
  return drush_invoke_process($context, $command, $arguments, $data, array('method' => $mode, 'integrate' => TRUE, 'dispatch-using-alias' => TRUE));
}

/**
 * Invoke a command in a new process, targeting the site specified by
 * the provided site alias record.
 *
 * Use this function instead of drush_backend_invoke_sitealias,
 * drush_backend_invoke_args, or drush_backend_invoke_command
 * (all obsolete in drush 5).
 *
 * @param array $site_alias_record
 *  The site record to execute the command on.  Use '@self' to run on the current site.
 * @param string $command_name
 *  The command to invoke.
 * @param array $commandline_args
 *  The arguments to pass to the command.
 * @param array $commandline_options
 *  The options (e.g. --select) to provide to the command.
 * @param $backend_options
 * - TRUE - integrate errors
 * - FALSE - do not integrate errors
 * - array - @see drush_backend_invoke_concurrent
 *     There are also several options that _only_ work when set in
 *     this parameter.  They include:
 *     - 'invoke-multiple'
 *        If $site_alias_record represents a single site, then 'invoke-multiple'
 *        will cause the _same_ command with the _same_ arguments and options
 *        to be invoked concurrently (e.g. for running concurrent batch processes).
 *     - 'concurrency'
 *        Limits the number of concurrent processes that will run at the same time.
 *        Defaults to '4'.
 *     - 'override-simulated'
 *        Forces the command to run, even in 'simulated' mode. Useful for
 *        commands that do not change any state on the machine, e.g. to fetch
 *        database information for sql-sync via sql-conf.
 *     - 'interactive'
 *        Overrides the backend invoke process to run commands interactively.
 *     - 'fork'
 *        Overrides the backend invoke process to run non blocking commands in 
 *        the background. Forks a new process by adding a '&' at the end of the 
 *        command. The calling process does not receive any output from the child 
 *        process. The fork option is used to spawn a process that outlives its
 *        parent.
 *
 * @return
 *   If the command could not be completed successfully, FALSE.
 *   If the command was completed, this will return an associative
 *   array containing the results of the API call.
 *   @see drush_backend_get_result()
 *
 * Do not change the signature of this function!  drush_invoke_process
 * is one of the key Drush APIs.  See http://drupal.org/node/1152908
 */
function _provision__drush_invoke_process($site_alias_record, $command_name, $commandline_args = array(), $commandline_options = array(), $backend_options = FALSE) {
  if (is_array($site_alias_record) && array_key_exists('site-list', $site_alias_record)) {
    $site_alias_records = drush_sitealias_resolve_sitespecs($site_alias_record['site-list']);
    $site_alias_records = drush_sitealias_simplify_names($site_alias_records);
    foreach ($site_alias_records as $alias_name => $alias_record) {
      $invocations[] = array(
        'site' => $alias_record,
        'command' => $command_name,
        'args' => $commandline_args,
      );
    }
  }
  else {
    $invocations[] = array(
      'site' => $site_alias_record,
      'command' => $command_name,
      'args' => $commandline_args);
    $invoke_multiple = drush_get_option_override($backend_options, 'invoke-multiple', 0);
    if ($invoke_multiple) {
      $invocations = array_fill(0, $invoke_multiple, $invocations[0]);
    }
  }
  return _provison__drush_backend_invoke_concurrent($invocations, $commandline_options, $backend_options);
}

/**
 * Execute a new local or remote command in a new process.
 *
 * n.b. Prefer drush_invoke_process() to this function.
 *
 * @param invocations
 *   An array of command records to exacute. Each record should contain:
 *   - 'site':
 *       An array containing information used to generate the command.
 *       - 'remote-host'
 *            Optional. A remote host to execute the drush command on.
 *       - 'remote-user'
 *            Optional. Defaults to the current user. If you specify this, you can choose which module to send.
 *       - 'ssh-options'
 *            Optional.  Defaults to "-o PasswordAuthentication=no"
 *       - 'path-aliases'
 *            Optional; contains paths to folders and executables useful to the command.
 *       - '%drush-script'
 *            Optional. Defaults to the current drush.php file on the local machine, and
 *            to simply 'drush' (the drush script in the current PATH) on remote servers.
 *            You may also specify a different drush.php script explicitly.  You will need
 *            to set this when calling drush on a remote server if 'drush' is not in the
 *            PATH on that machine.
 *   - 'command':
 *       A defined drush command such as 'cron', 'status' or any of the available ones such as 'drush pm'.
 *   - 'args':
 *       An array of arguments for the command.
 *   - 'options'
 *       Optional. An array containing options to pass to the remote script.
 *       Array items with a numeric key are treated as optional arguments to the
 *       command.
 *   - 'backend-options':
 *       Optional. Additional parameters that control the operation of the invoke.
 *       - 'method'
 *            Optional. Defaults to 'GET'.
 *            If this parameter is set to 'POST', the $data array will be passed
 *            to the script being called as a JSON encoded string over the STDIN
 *            pipe of that process. This is preferable if you have to pass
 *            sensitive data such as passwords and the like.
 *            For any other value, the $data array will be collapsed down into a
 *            set of command line options to the script.
 *       - 'integrate'
 *            Optional. Defaults to TRUE.
 *            If TRUE, any error statuses will be integrated into the current
 *            process. This might not be what you want, if you are writing a
 *            command that operates on multiple sites.
 *       - 'log'
 *            Optional. Defaults to TRUE.
 *            If TRUE, any log messages will be integrated into the current
 *            process.
 *       - 'output'
 *            Optional. Defaults to TRUE.
 *            If TRUE, output from the command will be synchronously printed to
 *            stdout.
 *       - 'drush-script'
 *            Optional. Defaults to the current drush.php file on the local
 *            machine, and to simply 'drush' (the drush script in the current
 *            PATH) on remote servers.  You may also specify a different drush.php
 *            script explicitly.  You will need to set this when calling drush on
 *            a remote server if 'drush' is not in the PATH on that machine.
 * @param common_options
 *    Optional. Merged in with the options for each invocation.
 * @param backend_options
 *    Optional. Merged in with the backend options for each invocation.
 * @param default_command
 *    Optional. Used as the 'command' for any invocation that does not
 *    define a command explicitly.
 * @param default_site
 *    Optional. Used as the 'site' for any invocation that does not
 *    define a site explicitly.
 * @param context
 *    Optional. Passed in to proc_open if provided.
 *
 * @return
 *   If the command could not be completed successfully, FALSE.
 *   If the command was completed, this will return an associative array containing the data from drush_backend_output().
 */
function _provison__drush_backend_invoke_concurrent($invocations, $common_options = array(), $common_backend_options = array(), $default_command = NULL, $default_site = NULL, $context = NULL) {
  $index = 0;

  // Slice and dice our options in preparation to build a command string
  $invocation_options = array();
  foreach ($invocations as $invocation)  {
    $site_record = isset($invocation['site']) ? $invocation['site'] : $default_site;
    // NULL is a synonym to '@self', although the latter is preferred.
    if (!isset($site_record)) {
      $site_record = '@self';
    }
    // If the first parameter is not a site alias record,
    // then presume it is an alias name, and try to look up
    // the alias record.
    if (!is_array($site_record)) {
      $site_record = drush_sitealias_get_record($site_record);
    }
    $command = isset($invocation['command']) ? $invocation['command'] : $default_command;
    $args = isset($invocation['args']) ? $invocation['args'] : array();
    $command_options = isset($invocation['options']) ? $invocation['options'] : array();
    $backend_options = isset($invocation['backend-options']) ? $invocation['backend-options'] : array();
    // If $backend_options is passed in as a bool, interpret that as the value for 'integrate'
    if (!is_array($common_backend_options)) {
      $integrate = (bool)$common_backend_options;
      $common_backend_options = array('integrate' => $integrate);
    }

    $command_options += $common_options;
    $backend_options += $common_backend_options;

    $backend_options = _drush_backend_adjust_options($site_record, $command, $command_options, $backend_options);

    // Insure that contexts such as DRUSH_SIMULATE and NO_COLOR are included.
    $command_options += _drush_backend_get_global_contexts($site_record);

    if (isset($site_record['#name'])) {
      list($post_options, $commandline_options, $drush_global_options) = _drush_backend_classify_options(array(), $command_options, $backend_options);
      $command = '@' . ltrim($site_record['#name'], '@') . ' ' . $command;
    }
    else {
      list($post_options, $commandline_options, $drush_global_options) = _drush_backend_classify_options($site_record, $command_options, $backend_options);
    }
    $site_record += array('path-aliases' => array());
    $site_record['path-aliases'] += array(
      '%drush-script' => NULL,
    );

    $site = (array_key_exists('#name', $site_record) && !array_key_exists($site_record['#name'], $invocation_options)) ? $site_record['#name'] : $index++;
    $invocation_options[$site] = array(
      'site-record' => $site_record,
      'command' => $command,
      'args' => $args,
      'post-options' => $post_options,
      'drush-global-options' => $drush_global_options,
      'commandline-options' => $commandline_options,
      'command-options' => $command_options,
      'backend-options' => $backend_options,
    );
  }

  // Calculate the length of the longest output label
  $max_name_length = 0;
  $label_separator = '';
  if (!array_key_exists('no-label', $common_options) && (count($invocation_options) > 1)) {
    $label_separator = array_key_exists('#label-separator', $common_options) ? $common_options['#label-separator'] : ' >> ';
    foreach ($invocation_options as $site => $item) {
      $backend_options = $item['backend-options'];
      if (!array_key_exists('#output-label', $backend_options)) {
        if (is_numeric($site)) {
          $backend_options['#output-label'] = ' * [@self.' . $site;
          $label_separator = '] ';
        }
        else {
          $backend_options['#output-label'] = $site;
        }
        $invocation_options[$site]['backend-options']['#output-label'] = $backend_options['#output-label'];
      }
      $name_len = strlen($backend_options['#output-label']);
      if ($name_len > $max_name_length) {
        $max_name_length = $name_len;
      }
      if (array_key_exists('#label-separator', $backend_options)) {
        $label_separator = $backend_options['#label-separator'];
      }
    }
  }
  // Now pad out the output labels and add the label separator.
  $reserve_margin = $max_name_length + strlen($label_separator);
  foreach ($invocation_options as $site => $item) {
    $backend_options = $item['backend-options'] + array('#output-label' => '');
    $invocation_options[$site]['backend-options']['#output-label'] = str_pad($backend_options['#output-label'], $max_name_length, " ") . $label_separator;
    if ($reserve_margin) {
      $invocation_options[$site]['drush-global-options']['reserve-margin'] = $reserve_margin;
    }
  }

  // Now take our prepared options and generate the command strings
  $cmds = array();
  foreach ($invocation_options as $site => $item) {
    $site_record = $item['site-record'];
    $command = $item['command'];
    $args = $item['args'];
    $post_options = $item['post-options'];
    $commandline_options = $item['commandline-options'];
    $command_options = $item['command-options'];
    $drush_global_options = $item['drush-global-options'];
    $backend_options = $item['backend-options'];
    $os = drush_os($site_record);
    // If the caller did not pass in a specific path to drush, then we will
    // use a default value.  For commands that are being executed on the same
    // machine, we will use DRUSH_COMMAND, which is the path to the drush.php
    // that is running right now.  For remote commands, we will run a wrapper
    // script instead of drush.php -- drush.bat on Windows, or drush on Linux.
    $drush_path = $site_record['path-aliases']['%drush-script'];
    $drush_command_path = drush_build_drush_command($drush_path, array_key_exists('php', $command_options) ? $command_options['php'] : NULL, $os, array_key_exists('remote-host', $site_record));
    $cmd = _drush_backend_generate_command($site_record, $drush_command_path . " " . _drush_backend_argument_string($drush_global_options, $os) . " " .  $command, $args, $commandline_options, $backend_options) . ' 2>&1';
    $cmds[$site] = array(
      'cmd' => $cmd,
      'post-options' => $post_options,
      'backend-options' => $backend_options,
    );
  }

  return _drush_backend_invoke($cmds, $common_backend_options, $context);
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
 * Normalise a context name, ensuring that it starts with one '@'.
 *
 * @param $name
 *   The context name to normalise.
 *
 * @return
 *   The normalised context name.
 */
function provision_normalise_context_name($name) {
  return '@' . ltrim($name, '@');
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
include_once('provision.service.inc');
include_once('provision.file.inc');

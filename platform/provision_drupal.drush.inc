<?php
/**
 * @file
 * Drupal specific functions for the provisioning framework.
 *
 * This module is responsible for the creation and maintenance of the drupal settings.php file, the sites directory structure
 * and all the install api code.
 */

function provision_drupal_drush_help($section) {
  switch ($section) {
    case 'error:PROVISION_CONFIG_NOT_VALID' :
      return dt('Config file could not be loaded.');
    case 'error:PROVISION_DRUPAL_SITE_INSTALLED' :
      return dt('Site has already been installed.');
    case 'error:PROVISION_DRUPAL_SITE_NOT_FOUND' :
      return dt('Site was not found.');
    case 'error:PROVISION_DRUPAL_INSTALL_FAILED' :
      return dt('Could not complete Drupal installation.');
    case 'error:PROVISION_DRUPAL_UPDATE_FAILED' :
      return dt('Could not complete Drupal update.');
    case 'error:PROVISION_BACKUP_PATH_NOT_FOUND' :
      return dt("Backup directory does not exist.");
    case 'error:PROVISION_DRUPAL_INSTALL_MISSING_REQUIREMENTS' :
      return dt("Could not meet the requirements for installing the drupal profile");
    case 'error:PROVISION_REQUIRES_URL' :
      return dt('You need to specify the URL argument for this command');
  }

}

function provision_drupal_drush_engine_drupal() {
  $engines = array();
  $engines['clear'] = array();
  $engines['cron_key'] = array();
  $engines['deploy'] = array();
  $engines['import'] = array();
  $engines['install'] = array();
  $engines['packages'] = array();
  $engines['verify'] = array();
  return $engines;
}


/**
 * Finalize the platform / site
 *
 * This will run only if there were no errors in any of the previous hooks,
 * and will allow us to cache the successful settings to the site.php/ drushrc.php
 * files for future runs.
 */
function provision_drupal_drush_exit() {
  $command = drush_get_command();
  $command = explode(" ", $command['command']);

  if (preg_match("/^provision-/", $command[0]) && drush_get_option('provision_save_config', TRUE)) {
    if (d()->type === 'site') {
      if (drush_get_option('installed')) {
        // Don't generate the drushrc.php on provision-save/delete commands.
        if (!preg_match("/^provision-(save|delete)/", $command[0])) {
          provision_save_site_data();
        }
      }
    }
    elseif (d()->type === 'platform') {
      // Don't generate the drushrc.php on provision-save/delete/git-clone commands.
      // Avoiding running on git-clone is needed in order to let it work with makefiles in the git repo.
      // If this function runs, the platform directory is created, and drush make fails because the directory already exists.
      // @TODO: Determine what tasks should save platform data and only act on those.
      if (!preg_match("/^provision-(save|delete|git-clone)/", $command[0])) {
        provision_save_platform_data();
      }
    }
  }
}

/**
 * Test to see if the site settings.php exists
 *
 * @return
 *   If the file exists, return TRUE, else return FALSE.
 */
function _provision_drupal_site_exists() {
  return is_readable(d()->site_path . '/settings.php');
}

/**
 * This command does the actual installation in it's own thread,
 * so we can recover gracefully if things go really wrong.
 */
function drush_provision_drupal_provision_install_backend() {
  drush_include_engine('drupal', 'install');
}

/**
 * Sync the current Drupal platform and, if applicable, site. Call after
 * finishing operations that affect the filesystem.
 *
 * @param boolean $override_slave_authority
 *    Overwrite e.g. the files directory on the slave.
 *    Useful when the master server has donw file operations, such as restoring a backup.
 */
function provision_drupal_push_site($override_slave_authority = FALSE) {

  provision_file()->create_dir(d()->server->http_platforms_path, dt("Platforms"), 0755);
  d()->server->sync(d()->server->http_platforms_path, array(
    'exclude' => d()->server->http_platforms_path . '/*',  // Make sure remote directory is created
  ));

  // Sync the platform
  d()->service('http')->sync(d()->root, array('exclude-sites' => TRUE));

  if (d()->type === 'site') {
    // Check whether we're hosted on a cluster, in which case, the master is
    // authoritative.
    $cluster = d()->platform->web_server->http_service_type == 'cluster' ? TRUE : FALSE;
    $options = array();
    if ($override_slave_authority || $cluster) {
      $exclude = NULL;
    }
    else {
      $exclude = 'files/*' . PATH_SEPARATOR . 'private/*';
    }

    // Store the current exclude-path option
    $old_exclude = NULL;
    if(!is_null($exclude)) {
      $old_exclude = drush_get_option('exclude-paths');
      drush_set_option('exclude-paths', $exclude);
    }

    // Sync all filesystem changes to the remote server.
    d()->service('http')->sync(d()->site_path, $options);

    // Reset the exclude-path option
    if(!is_null($exclude)) {
      if(empty($old_exclude)) {
        drush_unset_option('exclude-paths');
      } else {
        drush_set_option('exclude-paths', $old_exclude);
      }
    }
  }
}

/**
 * Sync the current Drupal site BACK from a slave. Call before
 * running operations that need files where the slave is authoritative.
 *
 * E.g. before a backup is made.
 *
 * @param string $alias The site alias to work on, defaults to the current site.
 */
function provision_drupal_fetch_site($alias = NULL) {

  $site = d($alias);
  // synch filesystem changes back from the remote server.
  $site->service('http')->fetch($site->site_path . '/files/');
  $site->service('http')->fetch($site->site_path . '/private/');
  $site->service('http')->fetch($site->site_path . '/modules/');
  $site->service('http')->fetch($site->site_path . '/themes/');
  $site->service('http')->fetch($site->site_path . '/libraries/');
  // Questionable... who is authoritive?
  $site->service('http')->fetch($site->site_path . '/local.settings.php');
}

/**
 * Generate a settings file for the site.
 */
function _provision_drupal_create_settings_file() {
  $config = new Provision_Config_Drupal_Settings(d()->name, drush_get_context('site'));
  $config->write();
}

/**
 * Create the directories needed to host a drupal site
 *
 * Also maintains permissions on existing directories.
 *
 * @param $url
 *   The url of the site being invoked.
  */
function _provision_drupal_create_directories($url = NULL) {
  if (is_null($url)) {
    if (d()->type == 'site') {
      $url = d()->uri;
    }
    else {
      $url = 'all';
    }
  }
  # those directories will be created and their modes changed
  $mkdir = array(
    "sites/$url"                 => 0755,
    # those should be writable by the aegir primary group to ease development
    "sites/$url/themes"          => 02775,
    "sites/$url/modules"         => 02775,
    "sites/$url/libraries"       => 02775, # http://drupal.org/node/496240
  );

  $chgrp = array();
  // special case: platform. do not handle files dir
  if ($url != 'all') {
    $mkdir["sites/$url/files"]            = 02770;
    $chgrp["sites/$url/files"]            = d('@server_master')->web_group;

    $mkdir["sites/$url/files/tmp"]        = 02770;
    $chgrp["sites/$url/files/tmp"]        = d('@server_master')->web_group;

    $mkdir["sites/$url/files/images"]     = 02770;
    $chgrp["sites/$url/files/images"]     = d('@server_master')->web_group;

    $mkdir["sites/$url/files/pictures"]   = 02770;
    $chgrp["sites/$url/files/pictures"]   = d('@server_master')->web_group;

    $mkdir["sites/$url/files/css"]        = 02770;
    $chgrp["sites/$url/files/css"]        = d('@server_master')->web_group;

    $mkdir["sites/$url/files/js"]         = 02770;
    $chgrp["sites/$url/files/js"]         = d('@server_master')->web_group;

    $mkdir["sites/$url/files/ctools"]     = 02770;
    $chgrp["sites/$url/files/ctools"]     = d('@server_master')->web_group;

    $mkdir["sites/$url/files/imagecache"] = 02770;
    $chgrp["sites/$url/files/imagecache"] = d('@server_master')->web_group;

    $mkdir["sites/$url/files/locations"]  = 02770;
    $chgrp["sites/$url/files/locations"]  = d('@server_master')->web_group;

    // d7 support
    $mkdir["sites/$url/private"]          = 02770;
    $chgrp["sites/$url/private"]          = d('@server_master')->web_group;

    $mkdir["sites/$url/private/files"]    = 02770;
    $chgrp["sites/$url/private/files"]    = d('@server_master')->web_group;

    $mkdir["sites/$url/private/temp"]     = 02770;
    $chgrp["sites/$url/private/temp"]     = d('@server_master')->web_group;

    $mkdir["sites/$url/files/styles"]     = 02770;
    $chgrp["sites/$url/files/styles"]     = d('@server_master')->web_group;

    // d8 support
    $mkdir["sites/$url/private/config"]   = 02770;
    $chgrp["sites/$url/private/config"]   = d('@server_master')->web_group;

    $mkdir["sites/$url/private/config/sync"] = 02770;
    $chgrp["sites/$url/private/config/sync"] = d('@server_master')->web_group;
  }

  // These paths should not have recursive operations performed on them.
  $chmod_not_recursive = $chgrp_not_recursive = array(
    "sites/$url",
    "sites/$url/files",
    "sites/$url/files/tmp",
    "sites/$url/files/images",
    "sites/$url/files/pictures",
    "sites/$url/files/css",
    "sites/$url/files/js",
    "sites/$url/files/ctools",
    "sites/$url/files/imagecache",
    "sites/$url/files/locations",
    "sites/$url/private",
    "sites/$url/private/files",
    "sites/$url/private/temp",
    "sites/$url/files/styles",
    "sites/$url/files/private",
    "sites/$url/private/config",
    "sites/$url/private/config/sync"
  );

  // Allow other commands to add or alter the directories to be created,
  // chmod'd or chgrp'd.
  // TODO: Figure out a better way to do this.
  drush_command_invoke_all_ref('provision_drupal_create_directories_alter', $mkdir, $url);
  drush_command_invoke_all_ref('provision_drupal_chgrp_directories_alter', $chgrp, $url);
  drush_command_invoke_all_ref('provision_drupal_chgrp_not_recursive_alter', $chgrp_not_recursive, $url);
  drush_command_invoke_all_ref('provision_drupal_chmod_not_recursive_alter', $chmod_not_recursive, $url);

  foreach ($mkdir as $path => $perm) {
    if (!is_dir($path)) {
      provision_file()->mkdir($path)
        ->succeed('Created <code>@path</code>')
        ->fail('Could not create <code>@path</code>', 'DRUSH_PERM_ERROR');
    }
  }
  foreach ($chgrp as $path => $group) {
    if ($group !== FALSE) {
      provision_file()->chgrp($path, $group, !in_array($path, $chgrp_not_recursive))
        ->succeed('Changed group ownership of <code>@path</code> to @gid')
        ->fail('Could not change group ownership <code>@path</code> to @gid');
    }
  }
  // Apply chmod AFTER chgrp because chgrp strips special permissions.
  foreach ($mkdir as $path => $perm) {
    if ($perm !== FALSE) {
      provision_file()->chmod($path, $perm, !in_array($path, $chmod_not_recursive))
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions <code>@path</code> to @perm');
    }
  }
}

/**
 * Call the core file_create_htaccess() functions.
 *
 * Unlink the files first to avoid permission issues.
 * If drupal already created the file then it's owned by e.g. www-data and aegir can not chmod those.
 *
 * @see https://drupal.org/SA-CORE-2013-003
 */
function _provision_drupal_ensure_htaccess_update() {

  if (drush_drupal_major_version() == 7) {

    // Copied from modules/system/system.install system_requirements()
    $htaccess_files['public://.htaccess'] = array(
      'directory' => variable_get('file_public_path', conf_path() . '/files'),
    );
    if ($private_files_directory = variable_get('file_private_path')) {
      $htaccess_files['private://.htaccess'] = array(
        'directory' => $private_files_directory,
      );
    }
    $htaccess_files['temporary://.htaccess'] = array(
      'directory' => variable_get('file_temporary_path', file_directory_temp()),
    );
    foreach ($htaccess_files as $htaccess_file => $info) {
      // Check for the string which was added to the recommended .htaccess file
      // in the latest security update.
      if (!file_exists($htaccess_file) || !($contents = @file_get_contents($htaccess_file)) || strpos($contents, 'Drupal_Security_Do_Not_Remove_See_SA_2013_003') === FALSE) {

        // Aegir specific
        @unlink($htaccess_file);
        file_create_htaccess($info['directory'], FALSE);
        drush_log(dt('Updated @file to match http://drupal.org/SA-CORE-2013-003', array('@file' => $info['directory'] . '/.htaccess')), 'notice');
        $path = file_stream_wrapper_get_instance_by_uri($htaccess_file)->getDirectoryPath();
        d()->service('http')->sync(d()->root . '/' . $info['directory'] . '/.htaccess');
      }
    }
  }
  elseif (drush_drupal_major_version() == 6 && function_exists('file_create_htaccess')) {

    // Copied from modules/system/system.install system_requirements()
    $htaccess_files['files_htaccess'] = array(
      'directory' => file_directory_path(),
    );
    $htaccess_files['temporary_files_htaccess'] = array(
      'directory' => file_directory_temp(),
    );
    foreach ($htaccess_files as $key => $info) {
      // Check for the string which was added to the recommended .htaccess file
      // in the latest security update.
      $htaccess_file = $info['directory'] . '/.htaccess';
      if (!file_exists($htaccess_file) || !($contents = @file_get_contents($htaccess_file)) || strpos($contents, 'Drupal_Security_Do_Not_Remove_See_SA_2013_003') === FALSE) {

        // Aegir specific
        @unlink($info['directory'] . '/.htaccess');
        file_create_htaccess($info['directory'], NULL);
        drush_log(dt('Updated @file to match http://drupal.org/SA-CORE-2013-003', array('@file' => $info['directory'] . '/.htaccess')), 'notice');
        d()->service('http')->sync(d()->root . '/' . $info['directory'] . '/.htaccess');
      }
    }
  }
}

/**
 * Runs an external script to reload all the various drupal caches
 */
function _provision_drupal_rebuild_caches() {
  if (d()->type === 'site') {
    drush_include_engine('drupal', 'clear');
  }
}

function provision_drupal_find_sites() {
  $sites = array();
  if ($dir = opendir("./sites")) {
    while (FALSE !== ($subdir = readdir($dir))) {
      // skip internal directory pointers
      if ($subdir != '.' && $subdir != '..') {
        $file = "./sites/$subdir/settings.php";
        if (file_exists("$file") && ($subdir != 'default') && !is_link("./sites/$subdir")) {
          $sites[$subdir] = $file;
        }
      }
    }
    closedir($dir);
  }
  else {
    drush_log(dt("Cannot find sites directory"), 'error');
    $sites = FALSE;
  }
  return $sites;
}

/**
 * Retrieve a list of aliases for the curent site.
 */
function provision_drupal_find_aliases() {
  $aliases = array();
  if (d()->type === 'site') {
    if (drush_drupal_major_version() >= 7) {
      $config = new Provision_Config_Drupal_Alias_Store(d()->name);
      $aliases = $config->find();
    }
    else {
      if ($dir = opendir(d()->root . "/sites")) {
        while (FALSE !== ($subdir = readdir($dir))) {
          // skip internal directory pointers
          if ($subdir != '.' && $subdir != '..') {
            $path = d()->root . '/sites/' . $subdir;
            if (is_link($path)) {
              if (d()->uri === readlink($path)) {
                $aliases[] = $subdir;
              }
            }
          }
        }
        closedir($dir);
      }
    }
  }

  return $aliases;
}

/**
 * Create and remove symlinks for each of the possible domain aliases of an
 * existing site.
 */
function _provision_drupal_maintain_aliases() {
  if (d()->type === 'site') {
    if (drush_drupal_major_version() >= 7) {
      $config = new Provision_Config_Drupal_Alias_Store(d()->name);
      $config->maintain();
      $config->write();
      d()->service('http')->sync($config->filename());
    }
    else {
      _provision_drupal_delete_aliases();
      if (!d()->redirection) {
        foreach (d()->aliases as $alias) {
          if ($alias = trim($alias)) {
            provision_file()->symlink(d()->uri, d()->root . '/sites/' . str_replace('/', '.', $alias))
              ->succeed('Created symlink for alias @target')
              ->fail('Could not create symlink for alias @target');
            d()->service('http')->sync(d()->root . '/sites/' . $alias);
          }
        }
      }
    }
  }
}

/**
 * Delete a list of aliases
 */
function _provision_drupal_delete_aliases() {

  if (d()->type === 'site') {
    if (drush_drupal_major_version() >= 7) {
        $config = new Provision_Config_Drupal_Alias_Store(d()->name);
        $config->delete();
        $config->write();
        d()->service('http')->sync($config->filename());
    }
    else {
      $aliases = provision_drupal_find_aliases();
      foreach ($aliases as $alias) {
        $path = d()->root . '/sites/' . $alias;
        provision_file()->unlink($path)
          ->succeed('Removed symlink for alias @path')
          ->fail('Could not remove symlink for alias @path');
        d()->service('http')->sync($path);
      }
    }
  }
}

function provision_find_packages() {
  // Load the version specific include files.
  drush_include_engine('drupal', 'packages', drush_drupal_major_version());

  $packages['base'] = _provision_find_packages('base');

  $packages['sites-all'] = _provision_find_packages('sites', 'all');

  // Create a package for the Drupal release
  $packages['base']['platforms'] = _provision_find_platforms();

  // Find install profiles.
  $profiles = _provision_find_profiles();
  drush_set_option('profiles', array_keys((array) $profiles), 'drupal');

  // Iterate through the install profiles, finding the profile specific packages
  foreach ($profiles as $profile => $info) {
    if (empty($info->version)) {
      $info->version = drush_drupal_version();
    }
    $packages['base']['profiles'][$profile] = $info;
    $packages['profiles'][$profile] =  _provision_find_packages('profiles', $profile);
  }

  return $packages;
}

function _provision_find_platforms() {
  return array(
    'drupal' => array(
    'short_name' => 'drupal', 'version' => drush_drupal_version(),
    'description' => dt("This platform is running @short_name @version", array('@short_name' => 'Drupal', '@version' => drush_drupal_version()))));
}

/**
 * A small helper function to reduce code duplication
 */
function _provision_find_packages($scope, $key = '') {
  $packages = array();
  $scope_text = ($key) ? "$scope/$key" : $scope;
  foreach (array('modules', 'themes') as $type) {
    $packages[$type] = array();
    $func = "_provision_drupal_find_$type";
    $result = $func($scope, $key);
    if (sizeof($result)) {
      $packages[$type] = $result;
      drush_log(dt("Found !count !type in !scope",
        array('!count' => sizeof($result),
        '!scope' => $scope_text, '!type' => $type)));
    }
  }
  return $packages;
}

/**
 * Map the system table to a packages multi-dimensional array component
 */
function provision_drupal_system_map() {
  // Load the version specific include files.
  drush_include_engine('drupal', 'packages');

  return _provision_drupal_system_map();
}

/**
 * Find modules in a certain scope.
 *
 * This function is general enough that it works for all supported
 * versions of Drupal.
 */
function _provision_drupal_find_modules($scope, $key = '') {
  $paths = _provision_drupal_search_paths($scope, $key, 'modules');
  $files = array();
  foreach ($paths as $path) {
    $files = array_merge($files, drush_scan_directory($path, "/\.module$/", array('.', '..', 'CVS', '.svn'), 0, TRUE, 'name'));
  }
  foreach ($files as $name => $info) {
    if (drush_drupal_major_version() >= 8) {
      $yaml_file = sprintf("%s/%s.info.yml", dirname($info->filename), $name);
      if(!file_exists($yaml_file)) {
        drush_log(dt("@name.info.yml not found.", array("@name" => $name)), 'notice');
        unset($files[$name]);
        continue;
      }
      $files[$name]->info = Symfony\Component\Yaml\Yaml::parse(file_get_contents($yaml_file));
      if (!empty($files[$name]->info['name'])) {
        $files[$name]->name = $files[$name]->info['name'];
      }
    }
    else {
      $info_file = sprintf("%s/%s.info", dirname($info->filename), $name);
      $files[$name]->info = provision_parse_info_file($info_file);
    }
    // Skip hidden modules
    if (isset($files[$name]->info['hidden']) && $files[$name]->info['hidden'] != FALSE) {
      unset($files[$name]);
      continue;
    }
    $install_file = sprintf("%s/%s.install", dirname($info->filename), $name);
    // Drupal now enforces a minimum schema version see: https://www.drupal.org/node/2186315
    if (drush_drupal_major_version() >= 8) {
      $schema_version = drush_drupal_major_version() * 1000;
    }
    else {
      $schema_version = 0;
    }
    if (file_exists($install_file)) {
      $source = file_get_contents(trim($install_file));
      $source = str_replace("\r\n", "\n", $source);
      $source = str_replace("\r", "\n", $source);
      $function_matches = array();
        preg_match_all('!function\s*&?([a-zA-Z0-9_]+)_update_([0-9]+)\s*\(.*?\s*\{!', $source, $function_matches);

      if (sizeof($function_matches[0])) {
        $schema_version = max($schema_version, max($function_matches[2]));
      }
    }
    $files[$name]->schema_version = $schema_version;
    $files[$name]->version = $files[$name]->info['version'];

    // Resolve the 'VERSION' constant used in Git checkouts.
    if ($files[$name]->version == 'VERSION') {
      $files[$name]->version = drush_drupal_version();
    }

  }
  return $files;
}

function provision_parse_info_file($filename) {
  $info = array();

  $defaults = array(
    'dependencies' => array(),
    'description' => '',
    'version' => NULL,
    'php' => DRUPAL_MINIMUM_PHP,
  );

  if (file_exists($filename)) {
    $info = _provision_drupal_parse_info_file($filename);
  }

  // Merge in defaults and return
  return $info + $defaults;
}

/**
 * Set up the $_SERVER environment variable so that drupal can correctly parse the settings.php file.
 * The real credentials are stored in the Apache vhost of the relevant site, to prevent leaking of
 * sensitive data to site administrators with PHP access who might otherwise access such credentials
 * potentially of other sites' settings.php in a multisite set-up.
 */
function provision_prepare_environment() {
  $fields = array('db_type', 'db_host', 'db_user', 'db_passwd', 'db_name', 'db_port');
  foreach ($fields as $key) {
    $_SERVER[$key] = drush_get_option($key, NULL, 'site');
  }

  // As of Drupal 7 there is no more mysqli type
  if (drush_drupal_major_version() >= 7) {
    $_SERVER['db_type'] = ($_SERVER['db_type'] == 'mysqli') ? 'mysql' : $_SERVER['db_type'];
  }
}


/**
 * Reload drushrc files (if available) from several possible locations.
 *
 * Because the base drush_load_config method only uses an include_once,
 * we run into issues when provision commands call other commands that
 * modify these config files.
 *
 * For the changes to become available, and more importantly passed to the
 * front end, we need to call this function after calling provision commands.
 */
function provision_reload_config($context, $file = NULL) {
  $file = ($file) ? $file : _drush_config_file($context);
  if (file_exists($file)) {
    drush_log("Reloading $context drushrc.php from $file");
    include($file);
    // $options will be defined by the config file included above.
    if (sizeof($options)) {
      $options = array_merge(drush_get_context($context, array()), $options);
      drush_set_context($context, $options);
    }
  }
}

/**
 * Maintain a symlink to the site within a client directory
 *
 * This creates a directory structure like this:
 *
 * ~/clients/foo/example.org -> ~/platforms/.../sites/example.org
 * ~/clients/bar/bar.example.com -> ~/platforms/.../sites/bar.example.com
 *
 * @todo this probably doesn't belong in this file
 */
function _provision_client_create_symlink() {
  if (d()->client_name) {
    $sites_dir = d()->server->clients_path . '/' . d()->client_name;
    provision_file()->create_dir($sites_dir, dt('Client home directory for @client', array('@client' => d()->client_name)), 0750);
    _provision_client_delete_old_symlink();
    provision_file()->symlink(d()->site_path, $sites_dir . '/' . d()->uri)
      ->succeed('Created symlink @path to @target')
      ->fail('Could not create symlink @path to @target: @reason');
  }
}

/**
 * Delete dangling symlinks for this site.
 *
 * This is a crude implementation, as we do not have the old client name so we
 * need to iterate over the directories. We only remove the first entry we
 * find to save some I/O.
 */
function _provision_client_delete_old_symlink() {
  $previous = d()->server->clients_path . '/' . d()->client_name . '/' . d()->uri;
  // this is necessary because unlink doesn't fail on missing files (!)
  $found = (is_file($previous) || is_link($previous));
  provision_file()->unlink($previous);
  if (!$found) {
    drush_log(dt("couldn't find previous client symlink, iterating through all sites"));
    // only iterate if the symlink location changed
    if ($dh = @opendir(d()->server->clients_path)) {
      while (($file = readdir($dh)) !== false) {
        if ($file != '.' && $file != '..') {
          $path = d()->server->clients_path . '/' . $file . '/' . d()->uri;
          if (is_file($path) || is_link($path)) {
            provision_file()->unlink($path);
            drush_log(dt("removed previous symlink in @path", array("@path" => $path)), 'success');
            break; // found it
          }
        }
      }
      closedir($dh);
    }
  }
}

/**
 * Delete the site symlink within the client directory
 *
 * This deletes the site symlink created on verify/install
 *
 * @see _provision_client_create_symlink()
 */
function _provision_client_delete_symlink() {
  if (d()->client_name) {
    provision_file()->unlink(d()->server->clients_path . '/' . d()->client_name . '/' . d()->uri)
      ->succeed('Deleted symlink @path')
      ->fail('Failed to delete symlink @path: @reason');
  }
}

/**
 * Find available languages for a profile.
 */
function _provision_find_profile_languages($profile_path) {
  $languages = array();

  // English is default.
  $languages['en'] = 1;

  // Find languages available.
  $files = array_keys(drush_scan_directory($profile_path . '/translations', '/\.po$/', array('.', '..', 'CVS'), 0, FALSE, 'filepath'));
  $files = array_merge($files, array_keys(drush_scan_directory($profile_path, '/\.po$/', array('.', '..', 'CVS'), 0, FALSE, 'filepath')));
  if (is_array($files)) {
    foreach ($files as $file) {
      if (preg_match('!(/|\.)([^\./]+)\.po$!', $file, $langcode)) {
        $languages[$langcode[2]] = 1; // use the language name as an index to weed out duplicates
      }
    }
  }
  return array_keys($languages);
}

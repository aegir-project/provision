<?php
// $Id$

include_once(dirname(__FILE__) . '/../provision.config.inc');

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
      // Don't generate the drushrc.php on provision-save/delete commands.
      if (!preg_match("/^provision-(save|delete)/", $command[0])) {
        provision_save_platform_data();
      }
    }
  }
}

/**
 * Test to see if the site settings.php exists
 *
 * @param url
 *   The url of the site to check
 * @return
 *   If the file exists, return TRUE, else return FALSE.
 */
function _provision_drupal_site_exists() {
  return file_exists(d()->site_path . '/settings.php');
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
 */
function provision_drupal_sync_site() {
  d()->service('http')->sync(d()->root, array('exclude-sites' => TRUE));
  if (d()->type === 'site') {
    // Sync all filesystem changes to the remote server.
    d()->service('http')->sync(d()->site_path, array('no-delete' => TRUE));
  }
}

/**
 * Generate a settings file for the site.
 *
 * @param url
 *   The url of the site being invoked.
 * @param data
 *   A reference to the associated array containing the data for the site. This needs to be a reference, 
 *   because the modules might provide additional information about the site.
 */
function _provision_drupal_create_settings_file() {
  $config = new provisionConfig_drupal_settings(d()->name, drush_get_context('site'));
  $config->write();
  provision_drupal_sync_site();
}

class provisionConfig_drupal_settings extends provisionConfig {
  public $template = 'provision_drupal_settings.tpl.php';
  public $description = 'Drupal settings.php file';
  public $creds = array();
  protected $mode = 0440;

  function filename() {
    return $this->site_path . '/settings.php';
  }

  function process() {
    if (drush_drupal_major_version() >= 7) {
      $this->data['db_type'] = ($this->data['db_type'] == 'mysqli') ? 'mysql' : $this->data['db_type'];
    }
    $this->version = provision_version();
    $this->api_version = provision_api_version();
    $this->cloaked = drush_get_option('provision_db_cloaking', $this->context->service('http')->cloaked_db_creds());

    foreach (array('db_type', 'db_user', 'db_passwd', 'db_host', 'db_name', 'db_port') as $key) {
      $this->creds[$key] = urldecode($this->data[$key]);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_drupal_config', d()->uri, $this->data));

    $this->group = $this->platform->server->web_group;

    // Add a handy variable indicating if the site is being backed up, we can
    // then react to this and change any settings we don't want backed up.
    $backup_file = drush_get_option('backup_file');
    $this->backup_in_progress = !empty($backup_file);
  }
}

/**
 * Create the directories needed to host a drupal site
 *
 * Also maintains permissions on existing directories.
 */
function _provision_drupal_create_directories($url = NULL, $profile = NULL) {
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
  }

  // These paths should not have recursive operations performed on them.
  $not_recursive = array(
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
    "sites/$url/private/temp"
  );

  foreach ($mkdir as $path => $perm) {
    if (!is_dir($path)) {
      provision_file()->mkdir($path) 
        ->succeed('Created <code>@path</code>')
        ->fail('Could not create <code>@path</code>', 'DRUSH_PERM_ERROR');
    }

    provision_file()->chmod($path, $perm, !in_array($path, $not_recursive))
      ->succeed('Changed permissions of <code>@path</code> to @perm')
      ->fail('Could not change permissions <code>@path</code> to @perm');
  }
  foreach ($chgrp as $path => $group) {
    provision_file()->chgrp($path, $group, !in_array($path, $not_recursive))
      ->succeed('Changed group ownership of <code>@path</code> to @gid')
      ->fail('Could not change group ownership <code>@path</code> to @gid');
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

/**
 * Find available profiles on this platform.
 */
function _provision_find_profiles() {
  include_once('includes/install.inc');

  if (!$dir = opendir("./profiles")) {
    drush_log(dt("Cannot find profiles directory"), 'error');
    return FALSE;
  }
  while (FALSE !== ($name = readdir($dir))) {
    $languages = array();
    $file = "./profiles/$name/$name.profile";
    if ($name == '..' || $name == '.' || !file_exists($file)) {
      continue;
    }
    $profile = new stdClass();
    $profile->name = $name;
    $profile->filename = $file;

    _provision_cvs_deploy($profile);

    $profile->info = array();

    $info_file = "./profiles/$name/$name.info";
    if (file_exists($info_file)) {
      $profile->info = provision_parse_info_file($info_file);
    }

    require_once($profile->filename);
    $func = $profile->name . "_profile_details";
    if (function_exists($func)) {
      $profile->info = array_merge($profile->info, $func());
    }

    $languages['en'] = 1;
    // Find languages available
    $files = array_keys(drush_scan_directory('./profiles/' . $name . '/translations', '/\.po$/', array('.', '..', 'CVS'), 0, FALSE, 'filepath'));
    $files = array_merge($files, array_keys(drush_scan_directory('./profiles/' . $name , '/\.po$/', array('.', '..', 'CVS'), 0, FALSE, 'filepath')));
    if (is_array($files)) {
      foreach ($files as $file) {
        if (preg_match('!(/|\.)([^\./]+)\.po$!', $file, $langcode)) {
          $languages[$langcode[2]] = 1; // use the language name as an index to weed out duplicates
        }
      }
    }
    $profile->info['languages'] = array_keys($languages);

    // Drupal 7 renamed the default install profile to 'standard'
    // Aegir now allows projects to specify an "old short name" to provide an upgrade path when projects get renamed. 
    if ($profile->name == 'standard') {
      $profile->info['old_short_name'] = 'default';
    }

    $return[$name] = $profile;
    drush_log(dt('Found install profile %name', array('%name' => $name)));
  }

  return $return;

}

function provision_drupal_find_sites() {
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
  } else {
    drush_log(dt("Cannot find sites directory"), 'error');
    $sites = FALSE;
  }
  return $sites;
}

function _provision_drupal_get_cvs_versions($files) {
  foreach ($files as $modulename => $file) {
      $project = array();
      $project['filename'] = $file->filename;
      $project['name'] = $file->name;
      $file->info['description'] = str_replace("\n", "", $file->info['description']);
      if (!isset($project['project'])) {
        $project['project'] = cvs_deploy_get_project_name($project);
      }
      _cvs_deploy_version_alter($file->info['version'], $project);
      $name = !empty($project['project']) ? $project['project'] : $modulename;
      $files[$name] = $file; 
  }

  return $files;
}

class provisionConfig_drupal_alias_store extends provisionConfig_data_store {
  public $template = 'provision_drupal_sites.tpl.php';
  public $description = 'Drupal sites.php file';
  public $key = 'sites';
  protected $mode = 0644;

  function filename() {
    return $this->root . '/sites/sites.php';
  }
  
  function maintain() {
    $this->delete();
    if (!$this->redirection) {
      foreach ($this->aliases as $alias) {
        $this->records[$alias] = $this->uri;
      }
    }
  }
  
  function delete() {
    foreach ($this->find() as $alias) {
      unset($this->records[$alias]);
      unset($this->loaded_records[$alias]);
    }
  }
  
  function find() {
    return array_keys($this->merged_records(), $this->uri);
  }
}

/**
 * Retrieve a list of aliases for the curent site.
 */
function provision_drupal_find_aliases() {
  $aliases = array();
  if (d()->type === 'site') {
    if (drush_drupal_major_version() >= 7) {
      $config = new provisionConfig_drupal_alias_store(d()->name);
      $aliases = $config->find();
    } else {
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
      $config = new provisionConfig_drupal_alias_store(d()->name);
      $config->maintain();
      $config->write();
      d()->service('http')->sync($config->filename());
    } else {
      _provision_drupal_delete_aliases();

      if (!d()->redirection) {
        foreach(d()->aliases as $alias) {
          if ($alias = trim($alias)) {
            provision_file()->symlink(d()->uri, d()->root . '/sites/' . $alias)
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
        $config = new provisionConfig_drupal_alias_store(d()->name);
        $config->delete();
        $config->write();
        d()->service('http')->sync($config->filename());
    } else {
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

require_once(dirname(__FILE__) . '/cvs_deploy.inc');

function provision_find_packages() {
  // Load the version specific include files.
  drush_include_engine('drupal', 'packages', drush_drupal_major_version());

  $packages['base'] = _provision_find_packages('base');

  // Create a package for the Drupal release
  $packages['base']['platforms'] = _provision_find_platforms();

  // Find install profiles.
  $profiles = _provision_find_profiles();
  drush_set_option('profiles', array_keys((array) $profiles), 'drupal'); 

  // Iterate through the install profiles, finding the profile specific packages
  foreach ($profiles as $profile => $info) {
    _provision_cvs_deploy($info);
    if (!$info->version) {
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
    'description' => dt("This platform is running @short_name @version", array('@short_name' => 'Drupal', '@version' => VERSION))));
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

  $profiles = _provision_find_profiles();
  foreach ($profiles as $profile => $info) {
    _provision_cvs_deploy($info);
    if (!$info->version) {
      $info->version = drush_drupal_version();
    }
    $profiles[$profile] = $info;
  }
  $packages['platforms'] = _provision_find_platforms();

  $profile = drush_get_option('profile');
  $packages['profiles'][$profile] = $profiles[$profile];
  $packages['profiles'][$profile]->status = 1;

  foreach (_provision_system_query("module") as $module) { 
    $frags = explode("/", $module->filename);
    // ignore site-specific modules
    if ($frags[0] == 'sites' && $frags[1] != 'all') {
      continue;
    }
    $info_file = sprintf("%s/%s.info", dirname($module->filename), $module->name);
    $module->info = provision_parse_info_file($info_file);

    _provision_cvs_deploy($module);
    $module->filename = realpath($module->filename);
    if ($module->schema_version == -1) {
      $module->schema_version = 0;
    }
    $packages['modules'][$module->name] = $module;
  }

  drush_log(dt("Found !count modules", array('!count' => sizeof($packages['modules']))));

  // XXX: mostly a copy-paste from above
  foreach (_provision_system_query("theme") as $theme) {
    $frags = explode("/", $theme->filename);
    // ignore site-specific themes
    if ($frags[0] == 'sites' && $frags[1] != 'all') {
      continue;
    }
    $info_file = sprintf("%s/%s.info", dirname($theme->filename), $theme->name);
    $theme->info = provision_parse_info_file($info_file);
    _provision_cvs_deploy($theme);
    $theme->filename = realpath($theme->filename);

    if ($theme->schema_version == -1) {
      $theme->schema_version = 0;
    }

    $packages['themes'][$theme->name] = $theme;
  }
  drush_log(dt("Found !count themes", array('!count' => sizeof($packages['themes']))));
  return $packages;
}

/**
 * Retrieve a list of paths to search in a certain scope
 */
function _provision_drupal_search_paths($scope, $key = '', $type = 'modules') {
  $searchpaths = array();
  $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
  switch ($scope) {
    case 'base' : 
      $searchpaths[] = sprintf("%s/%s", $drupal_root, $type);
      $searchpaths[] = sprintf("%s/sites/all/%s", $drupal_root, $type);
      break;
    default : 
      if ($key) { 
        $searchpaths[] = sprintf("%s/%s/%s/%s", $drupal_root, $scope, $key, $type);
      }
      break;
     
  }
  return $searchpaths;
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
    $files = array_merge($files, drush_scan_directory($path, "/\.module$/", array('.', '..', 'CVS', '.svn'), 0, true, 'name'));
  }
  foreach ($files as $name => $info) {
    $install_file = sprintf("%s/%s.install", dirname($info->filename), $name);
    $schema_version = 0;
    if (file_exists($install_file)) {
      $source = file_get_contents(trim($install_file));
      $source = str_replace("\r\n", "\n", $source);
      $source = str_replace("\r", "\n", $source);
      $function_matches = array();
        preg_match_all('!function\s*&?([a-zA-Z0-9_]+)_update_([0-9]+)\s*\(.*?\s*\{!', $source, $function_matches);
      
      if (sizeof($function_matches[0])) {
        $schema_version = max($function_matches[2]);
      }
    }
    $info_file = sprintf("%s/%s.info", dirname($info->filename), $name);
    $files[$name]->info = provision_parse_info_file($info_file);
    $files[$name]->schema_version = $schema_version;
    _provision_cvs_deploy($files[$name]);
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
    $_SERVER[$key] = drush_get_option($key, null, 'site');
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
function provision_reload_config($context, $file = null) {
  $file = ($file) ? $file : _drush_config_file($context);
  if (file_exists($file)) {
    drush_log("Reloading $context drushrc.php from $file");
    include($file);
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
    provision_file()->unlink($sites_dir . '/' . d()->uri); // deliberatly ignore errors
    provision_file()->symlink(d()->site_path, $sites_dir . '/' . d()->uri)
      ->succeed('Created symlink @path to @target')
      ->fail('Could not create symlink @path to @target: @reason');
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

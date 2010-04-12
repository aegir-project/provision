<?php
// $Id$
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
  $engines['install'] = array();
  $engines['import'] = array();
  $engines['deploy'] = array();
  $engines['clear'] = array();
  $engines['packages'] = array();
  $engines['verify'] = array();
  return $engines;
}

/**
 * Initialize the platform / site
 *
 * This function is executed by drush_invoke, and is responsible
 * for populating the $data context array
 */
function provision_drupal_drush_init($url = NULL) {
  $command = drush_get_command();
  $command = explode(" ", $command['command']);
  if (preg_match("/^provision-/", $command[0])) {

    define('PROVISION_CONTEXT_SITE', (d()->type == 'site') ? TRUE : FALSE);
    define('PROVISION_CONTEXT_PLATFORM', !PROVISION_CONTEXT_SITE);
  }
}

/**
 * Finalize the platform / site
 *
 * This will run only if there were no errors in any of the previous hooks,
 * and will allow us to cache the successful settings to the site.php/ drushrc.php
 * files for future runs.
 */
function provision_drupal_drush_exit($url = NULL) {
  $command = drush_get_command();
  $command = explode(" ", $command['command']);

  if (preg_match("/^provision-/", $command[0])) {
    if (PROVISION_CONTEXT_SITE) {
      if (drush_get_option('installed')) {
        drush_set_option('site_url', drush_get_option('site_url'), 'site');
        drush_set_option('site_id', drush_get_option('site_id'), 'site');
        drush_set_option('client_email', drush_get_option('client_email'), 'site');
        provision_save_site_data($url);
      }
    }
    else {
      // Generate a drushrc.php for the platform unless it's being deleted
      if (!preg_match("/^provision-delete/", $command[0])) {
        $exists = provision_service('file')->exists(drush_get_option('docroot_path') . '/drushrc.php')
          ->succeed('Found existing drushrc.php file')
          ->fail('Generating drushrc.php file')
          ->status();
        if ($exists) {
          provision_service('file')->chmod(drush_get_option('docroot_path') . '/drushrc.php', 0600)
            ->succeed('Changed permissions of drushrc.php to @perm')
            ->fail('Could not change permissions of drushrc.php to @perm');
        }

        drush_set_option('context-path', drush_get_option('context-path', null, 'drupal'));
        drush_unset_option('context-path', 'drupal');

        drush_set_option('config-file', drush_get_option('config-file', null, 'drupal'));
        drush_unset_option('config-file', 'drupal');

        drush_save_config('drupal');

        provision_service('file')->chmod(drush_get_option('docroot_path') . '/drushrc.php', 0400)
          ->succeed('Changed permissions of drushrc.php to @perm')
          ->fail('Could not change permissions of drushrc.php to @perm');
      }
    }
  }
}

/**
 * Some commands need to have a url to operate on.
 *  This prints out a message to that effect.
 */
function _provision_drupal_url_required() {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_error('PROVISION_REQUIRES_URL');
  }
}

/**
 * Validate a site exists, ie: has a settings.php file
 *
 * This will return an error for sites that haven't been created yet
 */
function _provision_drupal_valid_site() {
  if (PROVISION_CONTEXT_SITE) {
    if (!_provision_drupal_site_exists(drush_get_option('site_url'))) {
      drush_set_error('PROVISION_DRUPAL_SITE_NOT_FOUND');
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
function _provision_drupal_site_exists($url) {
  return file_exists("sites/$url/settings.php");
}


/**
 * Validate a site has been installed, by checking it's site.php file. 
 *
 * This will return an error for sites that haven't been installed yet
 */
function _provision_drupal_valid_installed_site() {
  if (PROVISION_CONTEXT_SITE) {
    if (!_provision_drupal_site_installed(drush_get_option('site_url'))) {
      drush_set_error('PROVISION_DRUPAL_SITE_NOT_FOUND');
    }
  }
}

function _provision_drupal_valid_not_installed_site() {
  if (PROVISION_CONTEXT_SITE) {
    if (_provision_drupal_site_installed(drush_get_option('site_url'))) {
      drush_set_error('PROVISION_DRUPAL_SITE_INSTALLED');
    }
  }
}

/**
 * Test to see if the site has a site.php and has it set to 'installed'
 */
function _provision_drupal_site_installed($url) {
  if (_provision_drupal_site_exists($url)) {
    provision_load_site_data($url);
    return drush_get_option('installed');
  }
  return FALSE;
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
  $config = new provisionConfig_drupal_settings(drush_get_merged_options());
  $config->write();
}

class provisionConfig_drupal_settings extends provisionConfig {
  public $template = 'provision_drupal_settings.tpl.php';
  public $description = 'Drupal settings.php file';
  protected $mode = 0440;

  function filename() {
    return $this->data['sites_path'] . '/' . $this->data['site_url'] . '/settings.php';
  }

  function process() {
    if (drush_drupal_major_version() >= 7) {
      $this->data['db_type'] = ($this->data['db_type'] == 'mysqli') ? 'mysql' : $this->data['db_type'];
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_drupal_config', $this->data['site_url'], $this->data));

    $this->group = drush_get_option('web_group');
  }
}

/**
 * Create the directories needed to host a drupal site
 * 
 * Also maintains permissions on existing directories.
 */
function _provision_drupal_create_directories($url, $profile = NULL) {  
  # those directories will be created and their modes changed
  $mkdir = array(
    "sites/$url"                 => 0755,
    # those should be writable by the aegir primary group to ease development
    "sites/$url/themes"          => 02775,
    "sites/$url/modules"         => 02775,  
    "sites/$url/libraries"       => 02775, # http://drupal.org/node/496240
    // d7 support
    "sites/$url/private"         => 0755,
    "sites/$url/private/files"   => 02770,
    "sites/$url/private/temp"    => 02770,
  );
  $chown = array();
  $chgrp = array();
  // special case: platform. do not handle files dir
  if ($url != 'all') {
    $mkdir["sites/$url/files"]          = 02770;
    $mkdir["sites/$url/files/tmp"]          = 02770;
    $mkdir["sites/$url/files/images"]          = 02770;
    $mkdir["sites/$url/files/pictures"]          = 02770;
    # the owner/group of those directories will be changed recursively
    $chown["sites/$url/files"]          = drush_get_option('script_user');
    $chgrp["sites/$url/files"]          = drush_get_option('web_group');
    $chown["sites/$url/private"]        = drush_get_option('script_user');
    $chgrp["sites/$url/private/files"]  = drush_get_option('web_group');
    $chgrp["sites/$url/private/temp"]   = drush_get_option('web_group');
  }

  foreach ($mkdir as $path => $perm) {
    if (!is_dir($path)) {
      provision_service('file')->mkdir($path) 
        ->succeed('Created <code>@path</code>')
        ->fail('Could not create <code>@path</code>', 'DRUSH_PERM_ERROR');
    }

    provision_service('file')->chmod($path, $perm, TRUE)
      ->succeed('Changed permissions of <code>@path</code> to @perm')
      ->fail('Could not change permissions <code>@path</code> to @perm');
  }
  foreach ($chown as $path => $owner) {
    provision_service('file')->chown($path, $owner, TRUE)
      ->succeed('Changed ownership of <code>@path</code>')
      ->fail('Could not change ownership <code>@path</code>', 'DRUSH_PERM_ERROR');
  }
  foreach ($chgrp as $path => $group) {
    provision_service('file')->chgrp($path, $group, TRUE)
      ->succeed('Changed group ownership of <code>@path</code>')
      ->fail('Could not change group ownership <code>@path</code>');
  }
}

/**
 * Runs an external script to reload all the various drupal caches
 */
function _provision_drupal_rebuild_caches($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
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
    require_once($profile->filename);
    $func = $profile->name . "_profile_details";
    if (function_exists($func)) {
      $profile->info =  $func();
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

/**
 * Create and remove symlinks for each of the possible domain aliases of an existing site
 */
function _provision_drupal_maintain_aliases($url) {
  if (PROVISION_CONTEXT_SITE) {
    // First we delete all the old aliases
    drush_set_option('aliases', drush_get_option('aliases'), 'site');
    _provision_drupal_delete_aliases(drush_get_option('aliases', array(), 'site'), false);

    if (!drush_get_option('redirection')) {
      $aliases = drush_get_option('aliases');
      if (!is_array($aliases)) {
        $aliases = explode(",", $aliases);
      }
      foreach($aliases as $alias) {
        if (trim($alias)) {
          provision_service('file')->symlink($url, drush_get_option('docroot_path') . '/sites/' . $alias)
            ->succeed('Created symlink for alias @target')
            ->fail('Could not create symlink for alias @target');
        }
      }
    }
  }
}

/**
 * Delete a list of aliases
 */
function _provision_drupal_delete_aliases($aliases, $silent = false) {
  if (!is_array($aliases)) {
    $aliases = explode(",", $aliases);
  }
  if ($silent) {
    $error = NULL;
  }
  else {
    $error = 'Could not remove symlink for alias @path';
  }
  foreach ($aliases as $alias) {
    if ($alias = trim($alias)) {
      provision_service('file')->unlink(drush_get_option('docroot_path') . '/sites/' . $alias)
        ->succeed('Removed symlink for alias @path')
        ->fail($error);
    }
  }
}

require_once('cvs_deploy.inc');

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
  $fields = array('db_type', 'db_host', 'db_user', 'db_passwd', 'db_name');
  foreach ($fields as $key) {
    $_SERVER[$key] = drush_get_option($key, null, 'site');
  }

  // As of Drupal 7 there is no more mysqli type
  if (drush_drupal_major_version() >= 7) {
    $_SERVER['db_type'] = ($_SERVER['db_type'] == 'mysqli') ? 'mysql' : $_SERVER['db_type'];
  }
}

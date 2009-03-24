<?php
// $Id$
/**
 * @file
 * Drupal specific functions for the provisioning framework.
 *
 * This module is responsible for the creation and maintenance of the drupal settings.php file, the sites directory structure
 * and all the install api code. 
 */

define('PROVISION_BACKUP_PATH_NOT_FOUND', 'PROVISION_BACKUP_PATH_NOT_FOUND');
define('PROVISION_BACKUP_FAILED', 'PROVISION_BACKUP_FAILED');
define('PROVISION_DRUPAL_INSTALL_FAILED', 'PROVISION_DRUPAL_INSTALL_FAILED');
define('PROVISION_DRUPAL_UPDATE_FAILED', 'PROVISION_DRUPAL_UPDATE_FAILED');
define('PROVISION_DRUPAL_INSTALL_MISSING_REQUIREMENTS', 'PROVISION_DRUPAL_INSTALL_MISSING_REQUIREMENTS');
define('PROVISION_DRUPAL_SITE_INSTALLED', 'PROVISION_DRUPAL_SITE_INSTALLED');
define('PROVISION_DRUPAL_SITE_NOT_FOUND', 'PROVISION_DRUPAL_SITE_NOT_FOUND');
define('PROVISION_CONFIG_NOT_VALID', 'PROVISION_CONFIG_NOT_VALID');
define('PROVISION_REQUIRES_URL', 'PROVISION_REQUIRES_URL');

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

/**
 * Initialize the platform / site
 *
 * This function is executed by provision_invoke, and is responsible
 * for populating the $data context array
 */
function provision_drupal_provision_init($url = NULL) {

  if ($url) {
    drush_set_option('uri' , 'http://' . $url);
    provision_load_site_data($url);
    drush_set_default('site_url', $url);
    drush_set_default('profile', 'default');
    drush_set_default('language', 'en');
    drush_set_default('aliases', array());
  }

  define('PROVISION_CONTEXT_SITE', ($url) ? TRUE : FALSE);
  define('PROVISION_CONTEXT_PLATFORM', !PROVISION_CONTEXT_SITE);
}

/**
 * Finalize the platform / site
 *
 * This will run only if there were no errors in any of the previous hooks,
 * and will allow us to cache the successful settings to the site.php/ drushrc.php
 * files for future runs.
 */
function provision_drupal_provision_finalize($url = NULL) {

  if (PROVISION_CONTEXT_SITE) {
    if (drush_get_option('installed')) {
      drush_set_option('site_url', drush_get_option('site_url'), 'site');
      drush_set_option('site_id', drush_get_option('site_id'), 'site');
      provision_save_site_data($url);
    }
  }
  else {
    _provision_generate_config();
  }
}

/**
 * Some commands need to have a url to operate on.
 *  This prints out a message to that effect.
 */
function _provision_drupal_url_required() {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_error(PROVISION_REQUIRES_URL);
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
      drush_set_error(PROVISION_DRUPAL_SITE_NOT_FOUND);
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
      drush_set_error(PROVISION_DRUPAL_SITE_NOT_FOUND);
    }
  }
}

function _provision_drupal_valid_not_installed_site() {
  if (PROVISION_CONTEXT_SITE) {
    if (_provision_drupal_site_installed(drush_get_option('site_url'))) {
      drush_set_error(PROVISION_DRUPAL_SITE_INSTALLED);
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
 * The default template to use while generating config files.
 *
 * @return
 *   The default template for the config file
 */
function _provision_drupal_default_template() {
  return file_get_contents(dirname(__FILE__) .'/provision_drupal_settings.tpl.php');
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
function _provision_drupal_create_settings_file($url = NULL) {
  $options = drush_get_merged_options();

  $options['extra_config'] = "# Extra configuration from modules:\n";
  foreach (drush_command_implements('provision_drupal_config') as $module) {
    $options['extra_config'] .= "# -- $module settings --\n";
    $options['extra_config'] .= module_invoke($module, 'provision_drupal_config', $url) . "\n";
  }


  drush_log(dt("Generate settings.php file"));
  if (provision_path("exists", "sites/$url/settings.php")) {
    provision_path("chmod", "sites/$url/settings.php", 0640,
      dt('Changed permissions of settings.php to @confirm'),
      dt('Could not change permissions of settings.php to @confirm'));
  }

  $fp = fopen("sites/$url/settings.php", "w");
  $text =  _provision_drupal_default_template();
  fwrite($fp, "<?php\n". provision_render_config($text, $options));
  fclose($fp);

  # Change the permissions of the file
  provision_path("chmod", "sites/$url/settings.php", 0440,
    dt('Changed permissions of settings.php to @confirm'),
    dt('Could not change permissions of settings.php to @confirm'));

  provision_path("chgrp", "sites/$url/settings.php", drush_get_option('web_group'),
    dt('Change group ownership of settings.php to @confirm'),
    dt('Could not change group ownership of settings.php to @confirm'));
}

/**
 * Create the directories needed to host a drupal site
 * 
 * Also maintains permissions on existing directories.
 */
function _provision_drupal_create_directories($url, $profile = NULL) {  
  $paths = array(
    "sites/$url"                 => 0755,
    "sites/$url/files"           => 02770,
    "sites/$url/files/tmp"       => 02770,
    "sites/$url/files/images"    => 02770,
    "sites/$url/files/pictures"  => 02770,
    "sites/$url/themes"          => 0755,
    "sites/$url/modules"         => 0755,  
  );
  $grps = array(
    "sites/$url/files",
    "sites/$url/files/tmp",
    "sites/$url/files/images",
    "sites/$url/files/pictures",
  );

  foreach ($paths as $path => $perm) {
    if (!is_dir($path)) {
      provision_path("mkdir", $path, TRUE, 
        dt("Created <code>@path</code>"),
        dt("Could not create <code>@path</code>"),
        DRUSH_PERM_ERROR);
    }

    provision_path("chmod", $path, $perm, 
      dt("Changed permissions of <code>@path</code> to @confirm"),
      dt("Could not change permissions <code>@path</code> to @confirm"),
      DRUSH_PERM_ERROR);
  }
  foreach ($grps as $path) {
      provision_path("chown", $path, drush_get_option('script_user'), 
      dt("Changed ownership of <code>@path</code>"),
      dt("Could not change ownership <code>@path</code>"),
      DRUSH_PERM_ERROR );
    provision_path("chgrp", $path, drush_get_option('web_group'),
      dt("Changed group ownership of <code>@path</code>"),
      dt("Could not change group ownership <code>@path</code>"));
  }
}

/**
 * Runs an external script to reload all the various drupal caches
 */
function _provision_drupal_rebuild_caches($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    #drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);

    provision_platform_include(dirname(__FILE__), 'clear');
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

    require_once($profile->filename);
    $func = $profile->name . "_profile_details";
    if (function_exists($func)) {
      $profile->info =  $func();
    }

    $languages['en'] = 1;
    // Find languages available
    $files = array_keys(drush_scan_directory('./profiles/' . $name . '/translations', '\.po$', array('.', '..', 'CVS'), 0, FALSE, 'filepath'));
    $files = array_merge($files, array_keys(drush_scan_directory('./profiles/' . $name , '\.po$', array('.', '..', 'CVS'), 0, FALSE, 'filepath')));
    if (is_array($files)) {
      foreach ($files as $file) {
        if (preg_match('!(/|\.)([^\./]+)\.po$!', $file, $langcode)) {
          $languages[$langcode[2]] = 1; // use the language name as an index to weed out duplicates
        }
      }
    }
    $profile->info['languages'] = array_keys($languages);
    $return[$name] = $profile;
    drush_log(dt('found install profile %name', array('%name' => $name)));
  }

  return $return;

}

function provision_drupal_find_sites() {
  if ($dir = opendir("./sites")) {
    while (FALSE !== ($subdir = readdir($dir))) {
      $file = "./sites/$subdir/settings.php";
      if (file_exists("$file") && ($subdir != 'default') && !is_link("./sites/$subdir")) {
        $sites[$subdir] = $file;
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
      if (empty($project['project'])) {
        $project['project'] = cvs_deploy_get_project_name($project);
      }
      _cvs_deploy_version_alter($file->info['version'], $project);
      $name = ($project['project']) ? $project['project'] : $modulename;
      $files[$name] = $file; 
  }

  return $files;
}

/**
 * Create and remove symlinks for each of the possible domain aliases of an existing site
 */
function _provision_drupal_maintain_aliases($url) {
  if (PROVISION_CONTEXT_SITE) {
    $old_aliases = drush_get_option('aliases', array(), 'site');
    /**
     * First we remove all the old aliases
     */
    if (is_array($old_aliases)) {
      _provision_drupal_delete_aliases($old_aliases);
    }


    $aliases = drush_get_option('aliases');
    if (is_array($aliases)) {
      foreach($aliases as $alias) {
        provision_path("symlink", $url, drush_get_option('docroot_path') . "/sites/" . $alias, 
          dt("Created symlink for alias @alias", array("@alias" => $alias)), 
          dt("Could not create symlink for alias @alias", array("@alias" => $alias)));
      }
    }
  }
}

/**
 * Delete a list of aliases
 */
function _provision_drupal_delete_aliases($aliases) {
  foreach ($aliases as $alias) {
    provision_path("unlink", drush_get_option('docroot_path') . "/sites/" . $alias, TRUE,
          dt("Removed symlink for alias @alias", array("@alias" => $alias)), 
          dt("Could not remove symlink for alias @alias", array("@alias" => $alias)));
  }
}

function provision_drupal_install_log($ret) {
  if (sizeof($ret)) {
    foreach ($ret as $info) {
      if (is_array($info)) {
        if (!$info['success']) {
          drush_set_error(PROVISION_DRUPAL_INSTALL_FAILED, $msg);
        }
        drush_log($info['query'], ($info['success']) ? 'success' : 'error');
      }
    }
  }
}


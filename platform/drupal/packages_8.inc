<?php
/**
 * @file
 * Package management code for Drupal 8.
 */

/**
 * Find themes in a certain scope.
 *
 * This function is based on _system_theme_data in Drupal 6 and Drupal 7.
 * We do not support, nor need information on subthemes at this point.
 */
function _provision_drupal_find_themes($scope, $key = '') {
  $paths = _provision_drupal_search_paths($scope, $key, 'themes');
  $files = array();
  $engines = array();
  foreach ($paths as $path) {
    $files = array_merge($files, drush_scan_directory($path, "/\.info\.yml$/", array('.', '..', 'CVS', '.svn'), 0, TRUE, 'name'));
    $engines = array_merge($engines, drush_scan_directory($path . "/engines", "/\.engine$/", array('.', '..', 'CVS', '.svn'), 0, TRUE, 'name'));
  }
  foreach ($files as $name => $file) {
    $files[$name]->info = Symfony\Component\Yaml\Yaml::parse($file->filename);
    if (!empty($files[$name]->info['name'])) {
      $files[$name]->name = $files[$name]->info['name'];
    }

    if (!empty($files[$name]->info['version'])) {
      $files[$name]->version = $files[$name]->info['version'];
    }

    // Resolve the 'VERSION' constant used in Git checkouts.
    if ($files[$name]->version == 'VERSION') {
      $files[$name]->version = drush_drupal_version();
    }

    // @TODO: This is copied from the D7 implementation and I'm almost certain
    // that is doesn't even do anything in D7.
    if (empty($files[$name]->info['engine'])) {
      $filename = dirname($files[$name]->filename) . '/' . $files[$name]->name . '.theme';
      if (file_exists($filename)) {
        $files[$name]->owner = $filename;
        $files[$name]->prefix = $name;
      }
    }
    else {
      $engine = $files[$name]->info['engine'];
      if (isset($engines[$engine])) {
        $files[$name]->owner = $engines[$engine]->filename;
        $files[$name]->prefix = $engines[$engine]->name;
        $files[$name]->template = TRUE;
      }
    }
  }
  return $files;
}

/**
 * Map the system table to a packages multi-dimensional array component.
 */
function _provision_drupal_system_map() {
  $profiles = _provision_find_profiles();
  foreach ($profiles as $profile => $info) {
    if (empty($info->version)) {
      $info->version = drush_drupal_version();
    }
    $profiles[$profile] = $info;
  }
  $packages['platforms'] = _provision_find_platforms();

  $profile = drush_get_option('profile');
  $packages['profiles'][$profile] = $profiles[$profile];
  $packages['profiles'][$profile]->status = 1;

  $packages['modules'] = _provision_system_query('module');
  drush_log(dt("Found !count modules", array('!count' => count($packages['modules']))));

  $packages['themes'] = _provision_system_query('theme');
  drush_log(dt("Found !count themes", array('!count' => count($packages['themes']))));

  return $packages;
}

/**
 * Query the core system for packages of a certain type.
 *
 * @param string $type
 *   E.g. module or theme.
 *
 * @return array
 *   List of packages.
 */
function _provision_system_query($type) {
  $packages = array();
  foreach (system_get_info($type) as $name => $package) {
    $package = (object) $package;
    $package->filename = drupal_get_filename($type, $name);
    $frags = explode("/", $package->filename);
    // Flag site-specific packages.
    if ($frags[0] == 'sites' && $frags[1] != 'all') {
      $package->platform = -1;
    }
    // In Drupal 8, system_get_info returns enabled modules/themes.
    $package->status = 1;

    $package->filename = realpath($package->filename);

    if ($type == 'module') {
      $package->schema_version = drupal_get_installed_schema_version($name);
    }

    $packages[$name] = $package;
  }

  return $packages;
}

/**
 * Find available profiles on this platform.
 */
function _provision_find_profiles() {
  $profiles = array();

  include_once('core/includes/install.inc');
  $profiles_subdirs[] = "./core/profiles";
  $profiles_subdirs[] = "./profiles";

  foreach($profiles_subdirs as $profiles_subdir) {
    if (!is_dir($profiles_subdir)) {
      drush_log(dt("Cannot find profiles directory"), 'error');
      return FALSE;
    }

    $info_files = drush_scan_directory($profiles_subdir, "/\.info.yml$/");
    foreach ($info_files as $path => $info_file) {
      $info = Symfony\Component\Yaml\Yaml::parse($path);
      if ($info['type'] == 'profile' && (!isset($info['hidden']) || !$info['hidden'])) {
        $profile = new stdClass();
        $profile->name = $info['name'];
        $profile->info = $info;
        $profile->filename = $path;
        $profile_path = dirname($path);
        $profile->info['languages'] = _provision_find_profile_languages($profile_path);

        $profiles[basename($profile_path)] = $profile;
        drush_log(dt('Found install profile %name', array('%name' => $name)));
      }
    }
  }
  return $profiles;
}


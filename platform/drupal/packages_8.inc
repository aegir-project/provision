<?php
/**
 * @file Package management code for Drupal 8
 */

/**
 * Find themes in a certain scope
 *
 * This function is based on _system_theme_data in Drupal 6 and Drupal 7.
 * We do not support, nor need information on subthemes at this point.
 */
function _provision_drupal_find_themes($scope, $key = '') {
  $paths = _provision_drupal_search_paths($scope, $key, 'themes');
  $files = array();
  $engines = array();
  foreach ($paths as $path) {
    $files = array_merge($files, drush_scan_directory($path, "/\.info\.yml$/", array('.', '..', 'CVS', '.svn'), 0, true, 'name'));
    $engines = array_merge($engines, drush_scan_directory($path . "/engines", "/\.engine$/", array('.', '..', 'CVS', '.svn'), 0, true, 'name'));
  }
  foreach ($files as $name => $file) {
    $files[$name]->info = Symfony\Component\Yaml\Yaml::parse($file->filename);
    if (!empty($files[$name]->info['name'])) {
      $files[$name]->name = $files[$name]->info['name'];
    }
    // @TODO: This is copied from the D7 implementation and I'm almost certain
    // that is doesn't even do anything in D7.
    if (empty($files[$name]->info['engine'])) {
      $filename = dirname($files[$name]->filename) .'/'. $files[$name]->name .'.theme';
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
    _provision_cvs_deploy($files[$name]);
  }
  return $files;
}

/**
 * Map the system table to a packages multi-dimensional array component
 */
function _provision_drupal_system_map() {
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


  $packages['modules'] = _provision_system_query('module');
  drush_log(dt("Found !count modules", array('!count' => sizeof($packages['modules']))));

  $packages['themes'] = _provision_system_query('theme');
  drush_log(dt("Found !count themes", array('!count' => sizeof($packages['themes']))));

  return $packages;
}

function _provision_system_query($type) {
  foreach (system_get_info($type) as $name => $package) {
    $package = (object) $package;
    $package->filename = drupal_get_filename($type, $name);
    $frags = explode("/", $package->filename);
    // flag site-specific packages
    if ($frags[0] == 'sites' && $frags[1] != 'all') {
      $packages->platform = -1;
    }

    /*_provision_cvs_deploy($module);*/
    $package->filename = realpath($package->filename);

    if ($type == 'module') {
      $package->schema_version = drupal_get_installed_schema_version($name);
    }

    $packages[$name] = $package;
  }

  return $packages;
}

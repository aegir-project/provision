<?php
/**
 * @file Package management code for Drupal 8
 */

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

<?php

/**
 * @file Package management code for Drupal 7
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
    $files = array_merge($files, drush_scan_directory($path, "/\.info$/", array('.', '..', 'CVS', '.svn'), 0, true, 'name'));
    $engines = array_merge($engines, drush_scan_directory($path . "/engines", "/\.engine$/", array('.', '..', 'CVS', '.svn'), 0, true, 'name'));
  }
  foreach ($files as $name => $file) {
    $files[$name]->info = _provision_drupal_parse_info_file($file->filename);
    if (!empty($files[$name]->info['name'])) {
      $files[$name]->name = $files[$name]->info['name'];
    }
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
 * This code is based on the Drupal 6 and Drupal 7 drupal_parse_info_file
 */
function _provision_drupal_parse_info_file($filename) {
  $info = array();
  $constants = get_defined_constants();

  if (!file_exists($filename)) {
    return $info;
  }

  $data = file_get_contents($filename);
  if (preg_match_all('
    @^\s*                           # Start at the beginning of a line, ignoring leading whitespace
    ((?:
      [^=;\[\]]|                    # Key names cannot contain equal signs, semi-colons or square brackets,
      \[[^\[\]]*\]                  # unless they are balanced and not nested
    )+?)
    \s*=\s*                         # Key/value pairs are separated by equal signs (ignoring white-space)
    (?:
      ("(?:[^"]|(?<=\\\\)")*")|     # Double-quoted string, which may contain slash-escaped quotes/slashes
      (\'(?:[^\']|(?<=\\\\)\')*\')| # Single-quoted string, which may contain slash-escaped quotes/slashes
      ([^\r\n]*?)                   # Non-quoted string
    )\s*$                           # Stop at the next end of a line, ignoring trailing whitespace
    @msx', $data, $matches, PREG_SET_ORDER)) {
    foreach ($matches as $match) {
      // Fetch the key and value string
      $i = 0;
      foreach (array('key', 'value1', 'value2', 'value3') as $var) {
        $$var = isset($match[++$i]) ? $match[$i] : '';
      }
      $value = stripslashes(substr($value1, 1, -1)) . stripslashes(substr($value2, 1, -1)) . $value3;

      // Parse array syntax
      $keys = preg_split('/\]?\[/', rtrim($key, ']'));
      $last = array_pop($keys);
      $parent = &$info;

      // Create nested arrays
      foreach ($keys as $key) {
        if ($key == '') {
          $key = count($parent);
        }
        if (!isset($parent[$key]) || !is_array($parent[$key])) {
          $parent[$key] = array();
        }
        $parent = &$parent[$key];
      }

      // Handle PHP constants.
      if (isset($constants[$value])) {
        $value = $constants[$value];
      }

      // Insert actual value
      if ($last == '') {
        $last = count($parent);
      }
      $parent[$last] = $value;
    }
  }

  return $info;
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

  foreach (_provision_system_query("module") as $module) {
    // Skip profiles, these have a separate section above.
    if (preg_match('/\.profile$/', $module->filename)) {
      continue;
    }
    $info_file = sprintf("%s/%s.info", dirname($module->filename), $module->name);
    $module->info = provision_parse_info_file($info_file);

    // Skip hidden modules
    if (isset($module->info['hidden']) && $module->info['hidden'] == 1) {
      continue;
    }
    $frags = explode("/", $module->filename);
    // flag site-specific modules
    if ($frags[0] == 'sites' && $frags[1] != 'all') {
      $module->platform = -1;
    }

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
    // flag site-specific themes
    if ($frags[0] == 'sites' && $frags[1] != 'all') {
      $theme->platform = -1;
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


function _provision_system_query($type) {
  if (file_exists(DRUPAL_ROOT . '/core/includes/drupal.inc')) {
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
  else {
    $entries = array();
    $result = db_query("SELECT * FROM {system} WHERE type=:type", array(':type' => $type));
    foreach ($result as $entry) {
      $entries[] = $entry;
    }
    return $entries;
  }
}


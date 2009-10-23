<?php
// $Id$

/**
 * @file Package management code for Drupal 5
 */

function _provision_drupal_parse_info_file($filename) {
  return parse_ini_file($filename);
}

/**
 * Find themes in a certain scope
 *
 * This function is based on system_theme_data in Drupal 5.
 * We only support PHPTemplate based themes in this, the rest
 * are so uncommonly used, that it's almost not worth it.
 */
function _provision_drupal_find_themes($scope, $key = '') {
  $paths = _provision_drupal_search_paths($scope, $key, 'themes');
  $files = array();
  $themes = array();
  $templates = array();
  foreach ($paths as $path) {
    $themes = array_merge($themes, drush_scan_directory($path, "/\.theme$/", array('.', '..', 'CVS', '.svn'), 0, true, 'name'));
    $templates = array_merge($templates, drush_scan_directory($path, "/page.tpl.php$/", array('.', '..', 'CVS', '.svn'), 0, true, 'filename'));
  }

  foreach ($themes as $name => $file) {
    $files[$name] = $file;
  }
  foreach ($templates as $filename => $file) {
    // The directory in which the template is stored is the name of the theme.
    $name = basename(dirname($filename));
    $file->template = TRUE;
    $file->engine = 'phptemplate';
    $files[$name] = $file;
  }
  foreach ($files as $name => $theme) {
    // Now that we have the themes, let's get the sub styles.
    foreach (drush_scan_directory(dirname($theme->filename), '/style.css$/') as $style) {
        $style->style = TRUE;
        $style->template = isset($theme->template) ? $theme->template : FALSE;
        $style->name = basename(dirname($style->filename));
        $style->owner = $theme->filename;
        if (array_key_exists($style->name, $files)) {
          continue;
        }
        $files[$style->name] = $style;
    }
  }

  foreach ($files as $name => $file) {
    // Now we get the information about the themes and styles from cvs_deploy
    _provision_cvs_deploy($files[$name]);
  }

  return $files;
}


function _provision_system_query($type) {
  $entries = array();
  $result = db_query("SELECT * FROM {system} WHERE type='%s'", $type);
  while ($entry = db_fetch_object($result)) {
    $entries[] = $entry;
  }
  return $entries;
}

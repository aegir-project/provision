<?php
// $Id$

$new_url = d()->uri;
$old_url = drush_get_option('old_uri', $new_url, 'options');

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

drush_log(
  dt('Changed paths from sites/@old_url to sites/@new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));

db_query("UPDATE {files} SET filepath=replace(filepath, 'sites/%s', 'sites/%s')", $old_url, $new_url);
db_query("UPDATE {users} SET picture = replace(picture, 'sites/%s', 'sites/%s')", $old_url, $new_url);
variable_set('file_directory_path', "sites/$new_url/files");
variable_set('file_directory_temp', "sites/$new_url/files/tmp");

// Global theme settings paths.
if ($var = variable_get('theme_settings', FALSE)) {
  $var['logo_path'] = str_replace($old_url, $new_url, $var['logo_path']);
  $var['favicon_path'] = str_replace($old_url, $new_url, $var['favicon_path']);
  variable_set('theme_settings', $var);
}

$themes = list_themes();
foreach (array_keys($themes) as $theme) {
  // Update logo and favicon paths for each theme.
  if ($var = variable_get('theme_'. $theme .'_settings', FALSE)) {
    $var['logo_path'] = str_replace($old_url, $new_url, $var['logo_path']);
    $var['favicon_path'] = str_replace($old_url, $new_url, $var['favicon_path']);
    variable_set('theme_'. $theme .'_settings', $var);
  }
  // Update color module paths.
  if (module_exists('color')) {
    foreach (array('_files', '_logo', '_screenshot', '_stylesheets') as $suffix) {
      if ($var = variable_get('color_'. $theme . $suffix, FALSE)) {
        variable_set('color_'. $theme . $suffix, str_replace($old_url, $new_url, $var));
      }
    }
  }
}

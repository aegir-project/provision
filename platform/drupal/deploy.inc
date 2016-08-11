<?php

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

$new_url = d()->uri;
$old_url = drush_get_option('old_uri', $new_url, 'cli');
$old_url_pattern = '/^' . preg_quote('sites/' . $old_url, '/') . '/';
$old_url_abs = '//' . $old_url . '/sites/' . $old_url . '/files';
$new_url_abs = '//' . $new_url . '/sites/' . $new_url . '/files';

// Update paths for sites migrated from standalone to the multisite install.
db_query("UPDATE {files}       SET filepath   = REPLACE(filepath,   'sites/default', 'sites/%s')", $new_url);
db_query("UPDATE {users}       SET picture    = REPLACE(picture,    'sites/default', 'sites/%s')", $new_url);
db_query("UPDATE {boxes}       SET body       = REPLACE(body,       'sites/default', 'sites/%s')", $new_url);
db_query("UPDATE {node_revisions} SET body    = REPLACE(body,       'sites/default', 'sites/%s')", $new_url);
db_query("UPDATE {node_revisions} SET teaser  = REPLACE(teaser,     'sites/default', 'sites/%s')", $new_url);

// Update absolute URLs to files for sites cloned/migrated/renamed in the multisite install.
db_query("UPDATE {files}       SET filepath   = REPLACE(filepath,   '%s', '%s')", $old_url_abs, $new_url_abs);
db_query("UPDATE {users}       SET picture    = REPLACE(picture,    '%s', '%s')", $old_url_abs, $new_url_abs);
db_query("UPDATE {boxes}       SET body       = REPLACE(body,       '%s', '%s')", $old_url_abs, $new_url_abs);
db_query("UPDATE {node_revisions} SET body    = REPLACE(body,       '%s', '%s')", $old_url_abs, $new_url_abs);
db_query("UPDATE {node_revisions} SET teaser  = REPLACE(teaser,     '%s', '%s')", $old_url_abs, $new_url_abs);

// Update paths for sites cloned/migrated/renamed in the multisite install.
db_query("UPDATE {files}       SET filepath   = REPLACE(filepath,   'sites/%s', 'sites/%s')", $old_url, $new_url);
db_query("UPDATE {users}       SET picture    = REPLACE(picture,    'sites/%s', 'sites/%s')", $old_url, $new_url);
db_query("UPDATE {boxes}       SET body       = REPLACE(body,       'sites/%s', 'sites/%s')", $old_url, $new_url);
db_query("UPDATE {node_revisions} SET body    = REPLACE(body,       'sites/%s', 'sites/%s')", $old_url, $new_url);
db_query("UPDATE {node_revisions} SET teaser  = REPLACE(teaser,     'sites/%s', 'sites/%s')", $old_url, $new_url);

// File paths.
variable_set('file_directory_path', "sites/$new_url/files");
variable_set('file_directory_temp', "sites/$new_url/files/tmp");

// Default user picture.
if ($picture = variable_get('user_picture_default', FALSE)) {
  $picture = preg_replace($old_url_pattern, 'sites/' . $new_url, $picture);
  variable_set('user_picture_default', $picture);
}

// Global theme settings paths.
if ($var = variable_get('theme_settings', FALSE)) {
  $var['logo_path'] = preg_replace($old_url_pattern, 'sites/' . $new_url, $var['logo_path']);
  $var['favicon_path'] = preg_replace($old_url_pattern, 'sites/' . $new_url, $var['favicon_path']);
  variable_set('theme_settings', $var);
}

$themes = list_themes();
foreach (array_keys($themes) as $theme) {
  // Update logo and favicon paths for each theme.
  if ($var = variable_get('theme_' . $theme . '_settings', FALSE)) {
    $var['logo_path'] = preg_replace($old_url_pattern, 'sites/' . $new_url, $var['logo_path']);
    $var['favicon_path'] = preg_replace($old_url_pattern, 'sites/' . $new_url, $var['favicon_path']);
    variable_set('theme_' . $theme . '_settings', $var);
  }
  // Update color module paths.
  if (module_exists('color')) {
    foreach (array('_files', '_logo', '_screenshot', '_stylesheets') as $suffix) {
      if ($var = variable_get('color_' . $theme . $suffix, FALSE)) {
        variable_set('color_' . $theme . $suffix, preg_replace($old_url_pattern, 'sites/' . $new_url, $var));
      }
    }
  }
}

drush_log(
  dt('Changed paths from sites/@old_url to sites/@new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));


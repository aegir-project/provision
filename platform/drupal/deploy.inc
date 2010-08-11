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


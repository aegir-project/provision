<?php
// $Id$

$new_url = drush_get_option('site_url');
$old_url = drush_get_option('old_site_url');


/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

drush_log(
  dt('Changed paths from sites/@old_url to sites/@new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));

db_query("UPDATE {files} SET filepath=replace(filepath, 'sites/%', 'sites/%')", $old_url, $new_url);
db_query("UPDATE {users} SET picture = replace(picture, 'sites/%s', 'sites/%s')", $old_url, $new_url);
variable_set('files_directory_path', "sites/$new_url/files");
variable_set('files_directory_temp', "sites/$new_url/files/tmp");


<?php
// $Id$

require_once(dirname(__FILE__) . '/../provision.inc');

if (sizeof($argv) == 3) {
  // Fake the necessary HTTP headers that Drupal needs:
  $new_url = $argv[1];
  $data = provision_external_init($argv[1]);
  $old_url = $argv[2];
}
else {
  provision_set_error(PROVISION_FRAMEWORK_ERROR);
  provision_log("error", "USAGE: drupal_deply.php new_url old_url\n");
  provision_output($data);
}

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

provision_log('notice', 
  pt('Changed paths from sites/@old_url to sites/@new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));

db_query("UPDATE {files} SET filepath=replace(filepath, 'sites/%', 'sites/%')", $old_url, $new_url);
db_query("UPDATE {users} SET picture = replace(picture, 'sites/%s', 'sites/%s')", $old_url, $new_url);
variable_set('files_directory_path', "sites/$new_url/files");
variable_set('files_directory_temp', "sites/$new_url/files/tmp");
$data = array();
provision_output($data);


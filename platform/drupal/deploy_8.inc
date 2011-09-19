<?php
// $Id$

$new_url = drush_get_option('uri');
$old_url = drush_get_option('uri', 'site');

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

drush_log(
  dt('Changed paths from sites/@old_url to sites/@new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));

variable_set('file_public_path', "sites/$new_url/files");
variable_set('file_private_path', "sites/$new_url/private/files");
variable_set('file_temporary_path', "sites/$new_url/private/temp");


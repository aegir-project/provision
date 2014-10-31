<?php
// $Id$

$new_url = d()->uri;
/**
 * @deprecated in drush3 it's 'options', in drush 4 it's 'cli', drop
 * 'options' when we drop drush3 support
 */
$context = drush_get_context('cli') ? 'cli' : 'options';
$old_url = drush_get_option('old_uri', $new_url, $context);

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

if (db_table_exists('file_managed')) {
  db_query("UPDATE {file_managed} SET uri = REPLACE(uri, :old, :new)", array(':old' => 'sites/default', ':new' => 'sites/$new_url'));
  db_query("UPDATE {file_managed} SET uri = REPLACE(uri, :old, :new)", array(':old' => 'sites/$old_url', ':new' => 'sites/$new_url'));
}

if (db_table_exists('files')) {
  db_query("UPDATE {files} SET filepath = REPLACE(filepath, :old, :new)", array(':old' => 'sites/$old_url', ':new' => 'sites/$new_url'));
  db_query("UPDATE {files} SET filepath = REPLACE(filepath, :old, :new)", array(':old' => 'sites/default', ':new' => 'sites/$new_url'));
}

\Drupal::config('system.file')
  ->set('path.private', "sites/$new_url/private/files")
  ->set('path.temporary', "sites/$new_url/private/temp")
  ->save();

drush_log(
  dt('Changed paths from sites/@old_url to sites/@new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));


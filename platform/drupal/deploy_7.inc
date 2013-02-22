<?php

$new_url = d()->uri;
$new_url = 'sites/' . $new_url;
/**
 * @deprecated in drush3 it's 'options', in drush 4 it's 'cli', drop
 * 'options' when we drop drush3 support
 */
$context = drush_get_context('cli') ? 'cli' : 'options';
$old_url = drush_get_option('old_uri', $new_url, $context);
$old_url = 'sites/' . $old_url;
$def_url = 'sites/default';
$bad_url = 'sites/$new_url';

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

if (db_table_exists('file_managed')) {
  db_query('UPDATE {file_managed} SET uri = REPLACE(uri, :old, :new)', array(':old' => $bad_url, ':new' => $new_url));
  db_query('UPDATE {file_managed} SET uri = REPLACE(uri, :old, :new)', array(':old' => $def_url, ':new' => $new_url));
  db_query('UPDATE {file_managed} SET uri = REPLACE(uri, :old, :new)', array(':old' => $old_url, ':new' => $new_url));
}

if (db_table_exists('files')) {
  db_query('UPDATE {files} SET filepath = REPLACE(filepath, :old, :new)', array(':old' => $bad_url, ':new' => $new_url));
  db_query('UPDATE {files} SET filepath = REPLACE(filepath, :old, :new)', array(':old' => $def_url, ':new' => $new_url));
  db_query('UPDATE {files} SET filepath = REPLACE(filepath, :old, :new)', array(':old' => $old_url, ':new' => $new_url));
}

if (db_table_exists('field_data_body')) {
  db_query('UPDATE {field_data_body} SET body_value = REPLACE(body_value, :old, :new)', array(':old' => $bad_url, ':new' => $new_url));
  db_query('UPDATE {field_data_body} SET body_value = REPLACE(body_value, :old, :new)', array(':old' => $def_url, ':new' => $new_url));
  db_query('UPDATE {field_data_body} SET body_value = REPLACE(body_value, :old, :new)', array(':old' => $old_url, ':new' => $new_url));
  if (db_field_exists('field_data_body', 'body_summary')) {
    db_query('UPDATE {field_data_body} SET body_summary = REPLACE(body_summary, :old, :new)', array(':old' => $bad_url, ':new' => $new_url));
    db_query('UPDATE {field_data_body} SET body_summary = REPLACE(body_summary, :old, :new)', array(':old' => $def_url, ':new' => $new_url));
    db_query('UPDATE {field_data_body} SET body_summary = REPLACE(body_summary, :old, :new)', array(':old' => $old_url, ':new' => $new_url));
  }
}

if (db_table_exists('field_revision_body')) {
  db_query('UPDATE {field_revision_body} SET body_value = REPLACE(body_value, :old, :new)', array(':old' => $bad_url, ':new' => $new_url));
  db_query('UPDATE {field_revision_body} SET body_value = REPLACE(body_value, :old, :new)', array(':old' => $def_url, ':new' => $new_url));
  db_query('UPDATE {field_revision_body} SET body_value = REPLACE(body_value, :old, :new)', array(':old' => $old_url, ':new' => $new_url));
  if (db_field_exists('field_revision_body', 'body_summary')) {
    db_query('UPDATE {field_revision_body} SET body_summary = REPLACE(body_summary, :old, :new)', array(':old' => $bad_url, ':new' => $new_url));
    db_query('UPDATE {field_revision_body} SET body_summary = REPLACE(body_summary, :old, :new)', array(':old' => $def_url, ':new' => $new_url));
    db_query('UPDATE {field_revision_body} SET body_summary = REPLACE(body_summary, :old, :new)', array(':old' => $old_url, ':new' => $new_url));
  }
}

variable_set('file_public_path', "$new_url/files");
variable_set('file_private_path', "$new_url/private/files");
variable_set('file_temporary_path', "$new_url/private/temp");

drush_log(
  dt('Changed paths from @old_url to @new_url',
  array('@old_url' => $old_url, '@new_url' => $new_url)));

drush_log(
  dt('Fixed possible broken paths from @bad_url to @new_url',
  array('@bad_url' => $bad_url, '@new_url' => $new_url)));

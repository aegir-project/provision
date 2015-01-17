<?php

/**
 * @file
 *
 * Update paths inside database content for sites cloned/migrated/renamed.
 */

$new_url = drush_get_option('uri');
$old_url = drush_get_option('old_uri');

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

if ($new_url != $old_url) {
  $url_changed  = TRUE;
}

if ($url_changed) {
  _provision_platform_drupal_deploy_8_replace($old_url, $new_url);
  _provision_platform_drupal_deploy_8_replace('default', $new_url);

  // Update files paths
  \Drupal::configFactory()->getEditable('system.file')
    ->set('path.private', "sites/$new_url/private/files")
    ->set('path.temporary', "sites/$new_url/private/temp")
    ->save();

  // Detect and update default site name, if needed
  $old_name = \Drupal::config('system.site')->get('name');
  if ($old_name == $old_url) {
    \Drupal::configFactory()->getEditable('system.site')
      ->set('name', "$new_url")
      ->save();
  }
}

function _provision_platform_drupal_deploy_8_replace($old_url, $new_url) {
  // Update paths for sites cloned/migrated/renamed in the multisite install.
  $replace_patterns = array(
    ':old' => 'sites/' . $old_url,
    ':new' => 'sites/' . $new_url,
  );

  if(db_table_exists('block_custom')) {
    db_query("UPDATE {block_custom} SET body     = REPLACE(body,     :old, :new)", $replace_patterns);
  }

  if (function_exists('field_info_field_map')) {
    $field_map = field_info_field_map();
    foreach ($field_map as $key => $info) {
      $tablekey = db_escape_table($key);
      // Replace in all known text fields.
      if (in_array($info['type'], array('text_long', 'text_with_summary'))) {
        drush_log('Replacing in ' . $key, 'debug');
        db_query("UPDATE {field_data_${tablekey}} SET ${tablekey}_value     = REPLACE(${tablekey}_value,   :old, :new)", $replace_patterns);
        db_query("UPDATE {field_revision_${tablekey}} SET ${tablekey}_value = REPLACE(${tablekey}_value,   :old, :new)", $replace_patterns);
      }

      // Replace in the optional summary.
      if ($info['type'] == 'text_with_summary') {
        drush_log('Replacing summary in ' . $key, 'debug');
        db_query("UPDATE {field_data_${tablekey}} SET ${tablekey}_summary     = REPLACE(${tablekey}_summary, :old, :new)", $replace_patterns);
        db_query("UPDATE {field_revision_${tablekey}} SET ${tablekey}_summary = REPLACE(${tablekey}_summary, :old, :new)", $replace_patterns);
      }
    }
  }
  drush_log(
    dt('Changed paths from sites/@old_url to sites/@new_url',
    array('@old_url' => $old_url, '@new_url' => $new_url)));
}


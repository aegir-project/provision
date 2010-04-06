<?php

function drush_db_provision_restore_validate($url) {
  provision_service('db')->connect($url);
}

function drush_db_pre_provision_restore($url = NULL) {
  provision_service('db')->create_site_database($url);
}

function drush_db_provision_restore($url) {
  provision_service('db')->import_site_database($url);
}


function drush_db_pre_provision_restore_rollback($url = NULL) {
  provision_service('db')->destroy_site_db($url);

  $keys = array('db_name', 'db_passwd', 'db_user', 'db_host');

  //Restore the original database credentials of the site.
  // They were never truly lost, but the options in the 'process' context
  // were overriding them. By unsetting the temporary options, the originals
  // are automatically restored.
  foreach ($keys as $key) {
    drush_unset_option($key, 'process');
  }
}

// Rollback doesn't apply here yet. Unless we trigger a restore of the first dump
// made. Which could go on infinitely if something is really long.
function drush_db_post_provision_restore($url = NULL) {
  provision_service('file')->unlink(drush_get_option('sites_path') . '/' . $url . '/database.sql')
    ->succeed('Removed dump file @path after restoring from it')
    ->fail('Could not remove dump file @path', 'DRUSH_PERM_ERROR');

  provision_service('db')->destroy_site_database($url);

  // TODO - test this .. it's probably horribly broken .. deleting the newly restored db instead of the old one.

  // The new database credentials will be saved against the site now.
  drush_set_option('db_name', drush_get_option('db_name'), 'site');
  drush_set_option('db_type', drush_get_option('db_type'), 'site');
  drush_set_option('db_user', drush_get_option('db_user'), 'site');
  drush_set_option('db_host', drush_get_option('db_host'), 'site');
  drush_set_option('db_passwd', drush_get_option('db_passwd'), 'site');
}


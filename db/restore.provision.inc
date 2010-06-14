<?php

function drush_db_provision_restore_validate() {
  d()->service('db')->connect();
}

function drush_db_pre_provision_restore() {
  drush_set_option('old_db_creds', d()->service('db')->fetch_site_credentials());

  d()->service('db')->create_site_database();
}

function drush_db_provision_restore() {
  d()->service('db')->import_site_database(d()->site_path . '.restore/database.sql');
}


function drush_db_pre_provision_restore_rollback() {
  d()->service('db')->destroy_site_database();

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
function drush_db_post_provision_restore() {
  provision_file()->unlink(d()->site_path . '/database.sql')
    ->succeed('Removed dump file @path after restoring from it')
    ->fail('Could not remove dump file @path', 'DRUSH_PERM_ERROR');

  d()->service('db')->destroy_site_database(drush_get_option('old_db_creds'));

  // TODO - test this .. it's probably horribly broken .. deleting the newly restored db instead of the old one.

  // The new database credentials will be saved against the site now.
  drush_set_option('db_name', drush_get_option('db_name'), 'site');
  drush_set_option('db_type', drush_get_option('db_type'), 'site');
  drush_set_option('db_user', drush_get_option('db_user'), 'site');
  drush_set_option('db_host', drush_get_option('db_host'), 'site');
  drush_set_option('db_passwd', drush_get_option('db_passwd'), 'site');
}


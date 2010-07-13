<?php

function drush_db_provision_deploy_validate() {
  d()->service('db')->connect();
}

function drush_db_pre_provision_deploy() {
  d()->service('db')->create_site_database();
}

function drush_db_pre_provision_deploy_rollback() {
  d()->service('db')->destroy_site_database();
}

function drush_db_provision_deploy() {
  d()->service('db')->import_site_database();
}


// Rollback doesn't apply here yet. Unless we trigger a deploy of the first dump
// made. Which could go on infinitely if something is really long.
function drush_db_post_provision_deploy() {
  provision_file()->unlink(d()->site_path . '/database.sql')
    ->succeed('Removed dump file @path after restoring from it')
    ->fail('Could not remove dump file @path', 'DRUSH_PERM_ERROR');
}

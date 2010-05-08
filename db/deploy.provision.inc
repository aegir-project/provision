<?php

function drush_db_provision_deploy_validate() {
  provision_service('db')->connect();
}

function drush_db_provision_deploy() {
  provision_service('db')->create_site_database();
  provision_service('db')->import_site_database();
}


function drush_db_provision_deploy_rollback() {
  provision_service('db')->destroy_site_database();
}

// Rollback doesn't apply here yet. Unless we trigger a deploy of the first dump
// made. Which could go on infinitely if something is really long.
function drush_db_post_provision_deploy() {
  provision_service('file')->unlink(drush_get_option('sites_path') . '/' . drush_get_option('uri') . '/database.sql')
    ->succeed('Removed dump file @path after restoring from it')
    ->fail('Could not remove dump file @path', 'DRUSH_PERM_ERROR');
}

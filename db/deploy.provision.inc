<?php

function drush_db_provision_deploy_validate() {
  provision_service('db')->connect();
}

function drush_db_provision_deploy($url) {
  provision_service('db')->create_site_database($url);
  provision_service('db')->import_site_database($url);
}


function drush_db_provision_deploy_rollback($url = NULL) {
  provision_service('db')->destroy_site_database($url);
}

// Rollback doesn't apply here yet. Unless we trigger a deploy of the first dump
// made. Which could go on infinitely if something is really long.
function drush_db_post_provision_deploy($url) {
  provision_path('unlink', drush_get_option('sites_path') .'/'. $url .'/database.sql', TRUE,
    dt("Removed dump file @path after restoring from it"),
    dt("Could not remove dump file @path"), 'DRUSH_PERM_ERROR');
}

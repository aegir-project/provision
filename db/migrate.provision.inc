<?php

function drush_db_provision_migrate_validate() {
  d()->service('db')->connect();
}

// Deletes the old database credentials
function drush_db_post_provision_migrate() {
  provision_reload_config('site', d()->site_path . '/drushrc.php');
  d()->service('db')->destroy_site_database();
}

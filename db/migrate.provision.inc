<?php
// $Id$

function drush_db_provision_migrate_validate() {
  d()->service('db')->connect();
}

// Deletes the old database credentials
function drush_db_post_provision_migrate() {
  d()->service('db')->destroy_site_database();
}

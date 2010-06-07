<?php
// $Id$

function drush_db_provision_migrate_validate() {
  d()->service('db')->connect();
}

// Deletes the old database credentials
function drush_db_post_provision_migrate() {
  d()->service('db')->destroy_site_database(
    drush_get_option('db_name'),
    drush_get_option('db_user'),
    drush_get_option('db_passwd')
  );
}

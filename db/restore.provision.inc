<?php

function drush_db_provision_restore_validate() {
  d()->service('db')->connect();
}

function drush_db_post_provision_restore() {
  d()->service('db')->destroy_site_database();
}


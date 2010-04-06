<?php

function drush_db_pre_provision_backup($url = NULL) {
  provision_service('db')->dump_site_database($url);

}

function drush_db_pre_provision_backup_rollback($url = NULL) {
  provision_service('file')->unlink(drush_get_option('sites_path') . '/' . $url . '/database.sql')
    ->succeed('Deleted mysql dump from sites directory')
    ->fail('Could not delete mysql dump from sites directory');
}

function drush_db_post_provision_backup($url = NULL) {
  drush_db_pre_provision_backup_rollback($url);
}


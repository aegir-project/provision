<?php

function drush_db_pre_provision_backup() {
  provision_service('db')->generate_dump();

}

function drush_db_pre_provision_backup_rollback() {
  provision_service('file')->unlink(drush_get_option('sites_path') . '/' . drush_get_option('uri') . '/database.sql')
    ->succeed('Deleted mysql dump from sites directory')
    ->fail('Could not delete mysql dump from sites directory');
}

function drush_db_post_provision_backup() {
  drush_db_pre_provision_backup_rollback();
}


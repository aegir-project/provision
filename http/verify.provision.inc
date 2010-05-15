<?php

function drush_http_provision_verify() {
  provision_service('http')->verify();
}

function drush_http_post_provision_verify() {
  if (PROVISION_CONTEXT_SERVER) {
    drush_set_option('master_url', drush_get_option('master_url'), 'user');
    drush_set_option('script_user', drush_get_option('script_user'), 'user');
    drush_set_option('backup_path', drush_get_option('backup_path'), 'user');
    drush_set_option('config_path', drush_get_option('config_path'), 'user');
  }
}

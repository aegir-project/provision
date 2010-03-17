<?php

function drush_provision_apache_provision_verify($url = null) {
  provision_service('http')->verify($url);
}

function drush_provision_apache_post_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('master_url', drush_get_option('master_url'), 'drupal');
    drush_set_option('publish_path', realpath(drush_get_option('publish_path')), 'drupal');
    drush_set_option('web_host', drush_get_option('web_host', '127.0.0.1'), 'drupal');
    drush_set_option('web_ip', drush_get_option('web_ip', ''), 'drupal');
    drush_set_option('web_group', drush_get_option('web_group'), 'drupal');
    drush_set_option('script_user', drush_get_option('script_user'), 'drupal');
    drush_set_option('restart_cmd', drush_get_option('restart_cmd'), 'drupal');
    drush_set_option('backup_path', drush_get_option('backup_path'), 'drupal');
    drush_set_option('config_path', drush_get_option('config_path'), 'drupal');
  }
}

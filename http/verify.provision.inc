<?php

function drush_http_provision_verify($url = null) {
  provision_service('http')->verify($url);
}

function drush_http_post_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('publish_path', realpath(drush_get_option('publish_path')), 'drupal');
    drush_unset_option('master_url',  'drupal');
    drush_unset_option('web_host', 'drupal');
    drush_unset_option('web_ip', 'drupal');
    drush_unset_option('web_group', 'drupal');
    drush_unset_option('script_user', 'drupal');
    drush_unset_option('restart_cmd', 'drupal');
    drush_unset_option('backup_path', 'drupal');
    drush_unset_option('config_path', 'drupal');
  }
  elseif (PROVISION_CONTEXT_SERVER) {
    drush_set_option('master_url', drush_get_option('master_url'), 'home.drush');
    drush_set_option('web_host', drush_get_option('web_host', '127.0.0.1'), 'home.drush');
    drush_set_option('web_ip', drush_get_option('web_ip', ''), 'home.drush');
    drush_set_option('web_group', drush_get_option('web_group'), 'home.drush');
    drush_set_option('script_user', drush_get_option('script_user'), 'home.drush');
    drush_set_option('restart_cmd', drush_get_option('restart_cmd'), 'home.drush');
    drush_set_option('backup_path', drush_get_option('backup_path'), 'home.drush');
    drush_set_option('config_path', drush_get_option('config_path'), 'home.drush');
  }
}

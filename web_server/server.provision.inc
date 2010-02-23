<?php

/**
 * Webserver verification task
 */

function drush_provision_apache_provision_server() {
  _provision_create_dir(drush_get_option('vhost_path'), dt("Virtual host configuration"), 0700);
  _provision_create_dir(drush_get_option('platform_conf_path'), dt("Platforms configuration"), 0700);
  _provision_apache_create_server_config();
  _provision_apache_restart_apache(TRUE);
}

function drush_provision_apache_post_provision_server() {
  drush_set_option('master_url', drush_get_option('master_url'), 'user');
  drush_set_option('web_host', drush_get_option('web_host', '127.0.0.1'), 'user');
  drush_set_option('web_ip', drush_get_option('web_ip', ''), 'user');
  drush_set_option('web_group', drush_get_option('web_group'), 'user');
  drush_set_option('script_user', drush_get_option('script_user'), 'user');
  drush_set_option('restart_cmd', drush_get_option('restart_cmd'), 'user');
  drush_set_option('backup_path', drush_get_option('backup_path'), 'user');
  drush_set_option('config_path', drush_get_option('config_path'), 'user');
}

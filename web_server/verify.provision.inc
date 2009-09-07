<?php

function drush_provision_apache_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    _provision_create_dir(drush_get_option('vhost_path'), dt("Virtual host configuration"), 0700);
    $data = drush_get_merged_options();
    $options = drush_get_merged_options();
    $data['extra_config'] = "# Extra configuration from modules:\n";
    $data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_dir_config', $data));
    _provision_apache_create_config('platform_' . $options['platform'] . '.conf', $data, _provision_apache_platform_template());
    _provision_apache_create_server_config($url);
  }
  else {
    _provision_apache_create_vhost_config($url);
  }
  _provision_apache_restart_apache(TRUE);
}

function drush_provision_apache_post_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('master_url', drush_get_option('master_url'), 'drupal');
    drush_set_option('publish_path', drush_get_option('publish_path'), 'drupal');
    drush_set_option('web_host', drush_get_option('web_host', '127.0.0.1'), 'drupal');
    drush_set_option('web_ip', drush_get_option('web_ip', ''), 'drupal');
    drush_set_option('web_group', drush_get_option('web_group'), 'drupal');
    drush_set_option('script_user', drush_get_option('script_user'), 'drupal');
    drush_set_option('restart_cmd', drush_get_option('restart_cmd'), 'drupal');
    drush_set_option('backup_path', drush_get_option('backup_path'), 'drupal');
    drush_set_option('config_path', drush_get_option('config_path'), 'drupal');
  }
}

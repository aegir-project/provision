<?php

function drush_provision_apache_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    if (drush_get_option('platform', null)) {
      _provision_apache_create_platform_config($url);
    }
  }
  else {
    _provision_apache_create_vhost_config($url);
  }
  _provision_apache_restart_apache(TRUE);
}

/**
 * Platform verification for webservers
 *
 * @deprecated the drush_unset_option() calls can be removed after 0.4
 */
function drush_provision_apache_post_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    drush_set_option('publish_path', realpath(drush_get_option('publish_path')), 'drupal');

    drush_log(dt("Removing server-specific settings from platform, make sure provision-server is ran"));
    drush_unset_option('master_url', 'drupal');
    drush_unset_option('web_host', 'drupal');
    drush_unset_option('web_ip', 'drupal');
    drush_unset_option('web_group', 'drupal');
    drush_unset_option('script_user', 'drupal');
    drush_unset_option('restart_cmd', 'drupal');
    drush_unset_option('backup_path', 'drupal');
    drush_unset_option('config_path', 'drupal');
  }
}

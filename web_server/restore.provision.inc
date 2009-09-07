<?php

/**
 * Implementation of hook_provision_pre_restore
 */
function drush_provision_apache_pre_provision_restore($url) {
  drush_set_option('redirect_url', drush_get_option('web_maintenance_url'));
  _provision_apache_create_vhost_config($url, _provision_apache_disabled_template());
  _provision_apache_restart_apache();
}

function drush_provision_apache_pre_provision_restore_rollback($url) {
  _provision_apache_create_vhost_config($url);
  _provision_apache_restart_apache();
}

function drush_provision_apache_post_provision_restore($url) {
  _provision_apache_create_vhost_config($url);
  _provision_apache_restart_apache();
}

// The old db details should have been switched back by the mysql rollback at this point.
function drush_provision_apache_post_provision_restore_rollback($url) {
  provision_apache_post_provision_restore($url);
}

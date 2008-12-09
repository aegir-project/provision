<?php

/**
 * Implementation of hook_provision_pre_restore
 */
function provision_apache_provision_pre_restore(&$data, $url = NULL) {
  $data['redirect_url'] = PROVISION_WEB_MAINTENANCE_URL;
  _provision_apache_create_vhost_config($data, $url, _provision_apache_redirect_template());
  _provision_apache_restart_apache();
}

function provision_apache_provision_pre_restore_rollback($data, $url = NULL) {
  _provision_apache_create_vhost_config($data, $url);
  _provision_apache_restart_apache();
}

function provision_apache_provision_post_restore(&$data, $url = NULL) {
  _provision_apache_create_vhost_config($data, $url);
  _provision_apache_restart_apache();
}

// The old db details should have been switched back by the mysql rollback at this point.
function provision_apache_provision_post_restore_rollback(&$data, $url = NULL) {
  provision_apache_provision_post_restore($data, $url);
}

<?php

/**
 * Implementation of hook_provision_pre_deploy
 */
function provision_apache_provision_pre_deploy(&$data, $url = NULL) {
  _provision_apache_create_vhost_config($data, $url);
}

function provision_apache_provision_pre_deploy_rollback($data, $url = NULL) {
  _provision_apache_delete_vhost_config($data, $url);
}

function provision_apache_provision_post_deploy(&$data, $url = NULL) {
  _provision_apache_restart_apache();
}

// The old db details should have been switched back by the mysql rollback at this point.
function provision_apache_provision_post_deploy_rollback(&$data, $url = NULL) {
  provision_apache_provision_post_deploy($data, $url);
}

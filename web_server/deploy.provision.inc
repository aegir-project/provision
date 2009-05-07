<?php

function drush_provision_apache_post_provision_deploy($url = NULL) {
  _provision_apache_create_vhost_config($url);
  _provision_apache_restart_apache();
}

/**
 * Delete the config file created.
 */
function drush_provision_apache_post_provision_deploy_rollback($url = NULL) {
  _provision_apache_delete_vhost_config($url);
}

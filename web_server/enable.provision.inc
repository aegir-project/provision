<?php

/**
 * Implementation of hook_provision_enable
 */
function drush_provision_apache_provision_enable($url = NULL) {
  _provision_apache_create_vhost_config($url);
  _provision_apache_restart_apache();
}


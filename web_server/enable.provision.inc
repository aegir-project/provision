<?php

/**
 * Implementation of hook_provision_enable
 */
function provision_apache_provision_enable(&$data, $url = NULL) {
  _provision_apache_create_vhost_config($data, $url);
  _provision_apache_restart_apache();
}


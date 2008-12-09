<?php

/**
 * Implementation of hook_provision_disable
 */
function provision_apache_provision_disable(&$data, $url = NULL) {
  $data['redirect_url'] = PROVISION_WEB_DISABLE_URL;
  _provision_apache_create_vhost_config($data, $url, _provision_apache_redirect_template());
  _provision_apache_restart_apache();
}


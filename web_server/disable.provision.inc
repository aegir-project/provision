<?php

/**
 * Implementation of hook_provision_disable
 */
function provision_apache_provision_disable($url = NULL) {
  drush_set_option('redirect_url', drush_get_option('web_disable_url'));
  _provision_apache_create_vhost_config($url, _provision_apache_redirect_template());
  _provision_apache_restart_apache();
}


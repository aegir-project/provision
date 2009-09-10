<?php

/**
 * @file
 *
 * Drush hooks and commands for SSL stuff in the Aegir backend
 */

function provision_ssl_provision_apache_vhost_config($url, $options) {
  if ($options['site_port'] == 443) {
    return array("php_value session.cookie_secure 1", "SSLEngine On");
  } else {
    return NULL;
  }
}

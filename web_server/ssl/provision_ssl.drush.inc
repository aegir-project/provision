<?php

/**
 * @file
 *
 * Drush hooks and commands for SSL stuff in the Aegir backend
 */

function provision_ssl_provision_apache_vhost_config($url, $options) {
  drush_log("somewhere in provision_ssl, yay");
  if ($options['site_port'] == 443 || TRUE) {
    return array("php_value session.cookie_secure 1", "SSLEngine On");
  } else {
    return NULL;
  }
}

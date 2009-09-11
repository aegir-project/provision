<?php

/**
 * @file
 *
 * Drush hooks and commands for SSL stuff in the Aegir backend
 */

/**
 * Implementation of hook_provision_apache_vhost_config()
 *
 * Add SSL configuration to the virtual host and create redirection if
 * necessary (if the ssl_redirect option is set). This all takes
 * effect only if the ssl option is on
 *
 * The configuration we add to the virtual host are those two simple
 * lines:
 *
 * php_value session.cookie_secure 1
 * SSLEngine On
 */
function provision_ssl_provision_apache_vhost_config($url, $options) {
  if ($options['ssl']) {
    if ($options['ssl_redirect'] && provision_path('exists', drush_get_option('vhost_path') . '/' . $url . '_80', FALSE, NULL, t("The redirection port is not available, no redirection installed"))) {
      $newoptions = $options;
      $newoptions['site_port'] = 80;
      _provision_apache_create_config($url . '_80', $newoptions, _provision_apache_redirect_template());
    }
    return array("php_value session.cookie_secure 1", "SSLEngine On");
  } else {
    return NULL;
  }
}

/**
 * Implementation of hook_provision_apache_delete_vhost()
 *
 * This will delete the redirection vhost if it was created.
 */
function provision_ssl_provision_apache_delete_vhost($url, $options) {
  if ($options['ssl'] && $options['ssl_redirect']) {
    provision_path('unlink', drush_get_option('vhost_path') . '/' . $url . '_80', TRUE, NULL, t("Failed deleting redirection vhost."));
  }
}

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
  if (!empty($options['ssl'])) {
    if ($options['ssl_redirect']) {
      // That's pretty bad, but if we *don't* do that, the vhost is never updated after the first write
      // XXX: we need a better way to identify if this is legit
      if (provision_service('file')->exists(drush_get_option('vhost_path') . '/' . $url . '_80')->status()) {
        drush_log(dt('Overwriting existing vhost %url:80 with redirection', array('%url' => $url)), 'warning');
      }
      $newoptions = $options;
      // in the redirection template, the ServerName is the first alias in the list
      array_push($newoptions['aliases'], $options['site_url']);
      $newoptions['site_port'] = 80;
      provision_write_config('vhost_path', $url . '_80', _provision_apache_redirect_template(), $newoptions);
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
    provision_service('file')->unlink(drush_get_option('vhost_path') . '/' . $url . '_80')
      ->succeed('Failed deleting redirection vhost.');
  }
}

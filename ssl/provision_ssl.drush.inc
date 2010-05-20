<?php

include_once(dirname(__FILE__) . '/../provision.service.inc');
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
function provision_ssl_provision_apache_vhost_config($options) {
  if (!$options['ssl']) {
    if ($options['ssl_redirect']) {
      // That's pretty bad, but if we *don't* do that, the vhost is never updated after the first write
      // XXX: we need a better way to identify if this is legit
      $config = new provisionConfig_ssl_vhost(drush_get_merged_options());
      $config->write();
    }
    $newoptions = $options;
    $newoptions['site_port'] = 443;
    $newoptions['extra_config'] = "php_value session.cookie_secure 1\nSSLEngine On\n";
    provision_write_config(drush_get_option('vhost_path') . '/' . drush_get_option('uri') .  '_443', _provision_apache_default_template(), $newoptions);
  }
  return NULL;
}

class provisionConfig_ssl_vhost extends provisionConfig {
  public $template = '../http/apache/vhost_redirect.tpl.php';
  public $description = 'Redirect for SSL';

  function filename() {
    return $this->data['vhost_path'] . '/' . $this->data['site_url'] . '_80';
  }

  function process() {
    array_push($this->data['aliases'], $this->data['site_url']);
    $this->data['site_port'] = 80;
  }
}

/**
 * Implementation of hook_provision_apache_delete_vhost()
 *
 * This will delete the redirection vhost if it was created.
 */
function provision_ssl_provision_apache_delete_vhost($options) {
  if ($options['ssl'] && $options['ssl_redirect']) {
    provision_service('file')->unlink(drush_get_option('vhost_path') . '/' . drush_get_option('uri') . '_80')
      ->succeed('Failed deleting redirection vhost.');
  }
}

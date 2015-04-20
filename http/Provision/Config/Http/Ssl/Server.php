<?php

/**
 * Base class for SSL enabled server level config.
 */
class Provision_Config_Http_Ssl_Server extends Provision_Config_Http_Server {
  public $template = 'server_ssl.tpl.php';
  public $description = 'encryption enabled webserver configuration';

  function write() {
    parent::write();

    if ($this->ssl_enabled && $this->ssl_key) {
      $path = dirname($this->data['ssl_cert']);
      // Make sure the ssl.d directory in the server ssl.d exists.
      provision_file()->create_dir($path,
      dt("Creating SSL Certificate directory for %key on %server", array(
        '%key' => $this->ssl_key,
        '%server' => $this->data['server']->remote_host,
      )), 0700);

      // Copy the certificates to the server's ssl.d directory.
      provision_file()->copy(
        $this->data['ssl_cert_source'],
        $this->data['ssl_cert'])
        ->succeed('Copied default SSL certificate into place')
        ->fail('Failed to copy default SSL certificate into place');
      provision_file()->copy(
        $this->data['ssl_cert_key_source'],
        $this->data['ssl_cert_key'])
        ->succeed('Copied default SSL key into place')
        ->fail('Failed to copy default SSL key into place');
      // Copy the chain certificate, if it is set.
      if (!empty($this->data['ssl_chain_cert_source'])) {
	      provision_file()->copy(
          $this->data['ssl_chain_cert_source'],
          $this->data['ssl_chain_cert'])
          ->succeed('Copied default SSL chain certificate key into place')
          ->fail('Failed to copy default SSL chain certificate into place');
      }
    }
  }
}

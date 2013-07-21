<?php

/**
 * Base class for SSL enabled virtual hosts.
 *
 * This class primarily abstracts the process of making sure the relevant keys
 * are synched to the server when the config files that use them get created.
 */
class Provision_Config_Http_Ssl_Site extends Provision_Config_Http_Site {
  public $template = 'vhost_ssl.tpl.php';
  public $disabled_template = 'vhost_ssl_disabled.tpl.php';

  public $description = 'encrypted virtual host configuration';

  function write() {
    parent::write();

    if ($this->ssl_enabled && $this->ssl_key) {
      $path = dirname($this->data['ssl_cert']);
      // Make sure the ssl.d directory in the server ssl.d exists. 
      provision_file()->create_dir($path, 
      dt("SSL Certificate directory for %key on %server", array(
        '%key' => $this->ssl_key,
        '%server' => $this->data['server']->remote_host,
      )), 0700);

      // Touch a file in the server's copy of this key, so that it knows the key is in use.
      // XXX: test. data structure may not be sound. try d($this->uri)
      // if $this fails
      Provision_Service_http_ssl::assign_certificate_site($this->ssl_key, $this);

      // Copy the certificates to the server's ssl.d directory.
      provision_file()->copy(
        $this->data['ssl_cert_source'],
        $this->data['ssl_cert'])
        || drush_set_error('SSL_CERT_COPY_FAIL', dt('failed to copy SSL certificate in place'));
      provision_file()->copy(
        $this->data['ssl_cert_key_source'],
        $this->data['ssl_cert_key'])
        || drush_set_error('SSL_KEY_COPY_FAIL', dt('failed to copy SSL key in place'));
      // Copy the chain certificate, if it is set.
      if (!empty($this->data['ssl_chain_cert_source'])) {
	      provision_file()->copy(
          $this->data['ssl_chain_cert_source'],
          $this->data['ssl_chain_cert'])
        || drush_set_error('SSL_CHAIN_COPY_FAIL', dt('failed to copy SSL certficate chain in place'));
      }
      // Sync the key directory to the remote server.
      $this->data['server']->sync($path, array(
       'exclude' => "{$path}/*.receipt",  // Don't need to synch the receipts
     ));
    }
  }

  /**
   * Remove a stale certificate file from the server.
   */
  function unlink() {
    parent::unlink();

    if ($this->ssl_enabled) {
      // XXX: to be tested, not sure the data structure is sound
      Provision_Service_http_ssl::free_certificate_site($this->ssl_key, $this);
    }
  }
  
  /**
   * Small utility function to stop code duplication.
   *
   * @deprecated unused
   * @see Provision_Service_http_ssl::free_certificate_site()
   */
  private function clear_certs($ssl_key) {
    return FALSE;
  }
}


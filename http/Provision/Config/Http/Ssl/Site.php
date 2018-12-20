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
  public $ssl_cert_ok = TRUE;

  public $description = 'encrypted virtual host configuration';

  function write() {
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
      if (!provision_file()->copy($this->data['ssl_cert_source'], $this->data['ssl_cert'])->status()) {
        drush_set_error('SSL_CERT_COPY_FAIL', dt('failed to copy SSL certificate in place'));
        $this->ssl_cert_ok = FALSE;
      }
      if (!provision_file()->copy($this->data['ssl_cert_key_source'], $this->data['ssl_cert_key'])->status()) {
        drush_set_error('SSL_KEY_COPY_FAIL', dt('failed to copy SSL key in place'));
        $this->ssl_cert_ok = FALSE;
      }
      // Copy the chain certificate, if it is set.
      if (!empty($this->data['ssl_chain_cert_source'])) {
        if (!provision_file()->copy($this->data['ssl_chain_cert_source'], $this->data['ssl_chain_cert'])->status()) {
          drush_set_error('SSL_CHAIN_COPY_FAIL', dt('failed to copy SSL certficate chain in place'));
          $this->ssl_cert_ok = FALSE;
        }
      }

      // If cert is not ok, turn off ssl_redirection.
      if ($this->ssl_cert_ok == FALSE) {
        $this->data['ssl_redirection'] = FALSE;
        drush_log(dt('SSL Certificate preparation failed. SSL has been disabled for this site.'), 'warning');
      }

      // Sync the key directory to the remote server.
      $this->data['server']->sync($path, array(
       'exclude' => "{$path}/*.receipt",  // Don't need to synch the receipts
     ));
    }

    // Call parent's write AFTER ensuring the certificates are in place to prevent
    // the vhost from referencing missing files.
    parent::write();
  }

  /**
   * Remove a stale certificate file from the server.
   */
  function unlink() {
    parent::unlink();

    if ($this->ssl_enabled) {
      // XXX: to be tested, not sure the data structure is sound
      //
      // ACHTUNG! This deletes even perfectly good certificate and key.
      // There is no check in place to determine if the cert is "stale".
      // Not sure what the idea was behind this cleanup, but it looks like
      // an unfinished work, aggressively deleting existing cert/key pair,
      // even if there is absolutely no reason to do so -- like when the site
      // is simply migrated to another platform, while its name doesn't change.
      //
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


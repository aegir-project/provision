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

    $ip_addresses = drush_get_option('site_ip_addresses', array(), 'site');
    if ($this->ssl_enabled && $this->ssl_key) {
      $path = dirname($this->data['ssl_cert']);
      // Make sure the ssl.d directory in the server ssl.d exists. 
      provision_file()->create_dir($path, 
      dt("SSL Certificate directory for %key on %server", array(
        '%key' => $this->ssl_key,
        '%server' => $this->data['server']->remote_host,
      )), 0700);

      // Touch a file in the server's copy of this key, so that it knows the key is in use.
      touch("{$path}/{$this->uri}.receipt");

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
    elseif ($ip = $ip_addresses[$this->data['server']->name]) {
      if ($ssl_key = Provision_Service_http_ssl::get_ip_certificate($ip, $this->data['server'])) {
        $this->clear_certs($ssl_key);
      }
    }
  }

  /**
   * Remove a stale certificate file from the server.
   */
  function unlink() {
    parent::unlink();

    $ip_addresses = drush_get_option('site_ip_addresses', array(), 'site');

    if ($this->ssl_enabled && $this->ssl_key) {
      $this->clear_certs($this->ssl_key);
    }
    elseif ($ip = $ip_addresses[$this->data['server']->name]) {
      if ($ssl_key = Provision_Service_http_ssl::get_ip_certificate($ip, $this->data['server'])) {
        $this->clear_certs($ssl_key);
      }
    }

  }
  
  /**
   * Small utility function to stop code duplication.
   */

  private function clear_certs($ssl_key) {
    $path = $this->data['server']->http_ssld_path . "/$ssl_key";

    // Remove the file system reciept we left for this file
    provision_file()->unlink("{$path}/{$this->uri}.receipt")->
        succeed(dt("Deleted SSL Certificate association stub for %site on %server", array(
          '%site' => $this->uri,
          '%server' => $this->data['server']->remote_host)));

    $used = Provision_Service_http_ssl::certificate_in_use($ssl_key, $this->data['server']);

    if (!$used) {
      // we can remove the certificate from the server ssl.d directory.
      _provision_recursive_delete($path);
      // remove the file from the remote server too.
      $this->data['server']->sync($path);

      // Most importantly, we remove the hold this cert had on the IP address.
      Provision_Service_http_ssl::free_certificate_ip($ssl_key, $this->data['server']);
    }
  }


}


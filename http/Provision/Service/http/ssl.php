<?php
/**
 * @file
 * The base implementation of the SSL capabale web service.
 */

/**
 * The base class for SSL supporting servers.
 *
 * In general, these function the same as normal servers, but have an extra
 * port and some extra variables in their templates.
 */
class Provision_Service_http_ssl extends Provision_Service_http_public {
  protected $ssl_enabled = TRUE;

  function default_ssl_port() {
    return 443;
  }

  function init_server() {
    parent::init_server();

    // SSL Port.
    $this->server->setProperty('http_ssl_port', $this->default_ssl_port());

    // SSL certificate store.
    // The certificates are generated from here, and distributed to the servers,
    // as needed.
    $this->server->ssld_path = "{$this->server->aegir_root}/config/ssl.d";

    // SSL certificate store for this server.
    // This server's certificates will be stored here.
    $this->server->http_ssld_path = "{$this->server->config_path}/ssl.d";
  }

  function init_site() {
    parent::init_site();

    $this->context->setProperty('ssl_enabled', 0);
    $this->context->setProperty('ssl_key', NULL);
    $this->context->setProperty('ip_addresses', array());
  }


  function config_data($config = NULL, $class = NULL) {
    $data = parent::config_data($config, $class);
    $data['http_ssl_port'] = $this->server->http_ssl_port;

    if ($config == 'site' && $this->context->ssl_enabled) {
      foreach ($this->context->ip_addresses as $server => $ip_address) {
        if ($server == $this->server->name || '@' . $server == $this->server->name) {
          $data['ip_address'] = $ip_address;
          break;
        }
      }
      if (!isset($data['ip_address'])) {
        drush_log(dt('No proper IP provided by the frontend for server %servername, using wildcard', array('%servername' => $this->server->name)), 'info');
        $data['ip_address'] = '*';
      }
      if ($this->context->ssl_enabled == 2) {
        $data['ssl_redirection'] = TRUE;
        $data['redirect_url'] = "https://{$this->context->uri}";
      }

      if ($ssl_key = $this->context->ssl_key) {
        // Retrieve the paths to the cert and key files.
        // they are generated if not found.
        $certs = $this->get_certificates($ssl_key);
        $data = array_merge($data, $certs);
      }
    }

    return $data;
  }

  /**
   * Retrieve an array containing the actual files for this ssl_key.
   *
   * If the files could not be found, this function will proceed to generate
   * certificates for the current site, so that the operation can complete
   * succesfully.
   */
  function get_certificates($ssl_key) {
    $source_path = "{$this->server->ssld_path}/{$ssl_key}";
    $certs['ssl_cert_key_source'] = "{$source_path}/openssl.key";
    $certs['ssl_cert_source'] = "{$source_path}/openssl.crt";

    foreach ($certs as $cert) {
      $exists = provision_file()->exists($cert)->status();
      if (!$exists) {
        // if any of the files don't exist, regenerate them.
        $this->generate_certificates($ssl_key);

        // break out of the loop.
        break;
      }
    }

    $path = "{$this->server->http_ssld_path}/{$ssl_key}";
    $certs['ssl_cert_key'] = "{$path}/openssl.key";
    $certs['ssl_cert'] = "{$path}/openssl.crt";

    // If a certificate chain file exists, add it.
    $chain_cert_source = "{$source_path}/openssl_chain.crt";
    if (provision_file()->exists($chain_cert_source)->status()) {
      $certs['ssl_chain_cert_source'] = $chain_cert_source;
      $certs['ssl_chain_cert'] = "{$path}/openssl_chain.crt";
    }
    return $certs;
  }

  /**
   * Generate a self-signed certificate for that key.
   *
   * Because we only generate certificates for sites we make some assumptions
   * based on the uri, but this cert may be replaced by the admin if they
   * already have an existing certificate.
   */
  function generate_certificates($ssl_key) {
    $path = "{$this->server->ssld_path}/{$ssl_key}";

    provision_file()->create_dir($path,
      dt("SSL certificate directory for %ssl_key", array(
        '%ssl_key' => $ssl_key
      )), 0700);

    if (provision_file()->exists($path)->status()) {
      drush_log(dt('generating 2048 bit RSA key in %path/', array('%path' => $path)));
      /* 
       * according to RSA security and most sites I could read, 1024
       * was recommended until 2010-2015 and 2048 is now the
       * recommended length for more sensitive data. we are therefore
       * taking the safest route.
       *
       * http://www.javamex.com/tutorials/cryptography/rsa_key_length.shtml
       * http://www.vocal.com/cryptography/rsa-key-size-selection/
       * https://en.wikipedia.org/wiki/Key_size#Key_size_and_encryption_system
       * http://www.redkestrel.co.uk/Articles/CSR.html
       */
      drush_shell_exec('openssl genrsa -out %s/openssl.key 2048', $path)
        || drush_set_error('SSL_KEY_GEN_FAIL', dt('failed to generate SSL key in %path', array('%path' => $path . '/openssl.key')));

      // Generate the CSR to make the key certifiable by third parties
      $ident = "/CN={$this->context->uri}/emailAddress=abuse@{$this->context->uri}";
      drush_shell_exec("openssl req -new -subj '%s' -key %s/openssl.key -out %s/openssl.csr -batch", $ident, $path, $path)
        || drush_log(dt('failed to generate signing request for certificate in %path', array('%path' => $path . '/openssl.csr')));

      // sign the certificate with itself, generating a self-signed
      // certificate. this will make a SHA1 certificate by default in
      // current OpenSSL.
      drush_shell_exec("openssl x509 -req -days 365 -in %s/openssl.csr -signkey %s/openssl.key  -out %s/openssl.crt", $path, $path, $path)
        || drush_set_error('SSL_CERT_GEN_FAIL', dt('failed to generate self-signed certificate in %path', array('%path' => $path . '/openssl.crt')));
    }
  }

  /**
   * Assign the given site to a certificate to mark its usage.
   *
   * This is necessary for the backend to figure out when it's okay to
   * remove certificates.
   *
   * Should never fail unless the receipt file cannot be created.
   *
   * @return the path to the receipt file if allocation succeeded
   */
  static function assign_certificate_site($ssl_key, $site) {
    $path = $site->platform->server->http_ssld_path . "/" . $ssl_key . "/" . $site->uri . ".receipt";
    drush_log(dt("registering site %site with SSL certificate %key with receipt file %path", array("%site" => $site->uri, "%key" => $ssl_key, "%path" => $path)));
    if (touch($path)) {
      return $path;
    }
    else {
      return FALSE;
    }
  }

  /**
   * Unallocate this certificate from that site.
   *
   * @return the path to the receipt file if removal was successful
   */
  static function free_certificate_site($ssl_key, $site) {
    if (empty($ssl_key)) return FALSE;
    $ssl_dir = $site->platform->server->http_ssld_path . "/" . $ssl_key . "/";
    // Remove the file system reciept we left for this file
    if (provision_file()->unlink($ssl_dir . $site->uri . ".receipt")->
        succeed(dt("Deleted SSL Certificate association receipt for %site on %server", array(
          '%site' => $site->uri,
          '%server' => $site->server->remote_host)))->status()) {
      if (!Provision_Service_http_ssl::certificate_in_use($ssl_key, $site->server)) {
        drush_log(dt("Deleting unused SSL directory: %dir", array('%dir' => $ssl_dir)));
        _provision_recursive_delete($ssl_dir);
        $site->server->sync($path);
      }
      return $path;
    }
    else {
      return FALSE;
    }
  }
  
  /**
   * Assign the certificate it's own distinct IP address for this server.
   *
   * Each certificate needs a unique IP address on each server in order
   * to be able to be encrypted.
   *
   * This code uses the filesystem by touching a reciept file in the
   * server's ssl.d directory.
   *
   * @deprecated this is now based the site URI
   * @see assign_certificate_site()
   */
  static function assign_certificate_ip($ssl_key, $server) {
    return FALSE;
  }

  /**
   * Remove the certificate's lock on the server's public IP.
   *
   * This function will delete the receipt file left behind by
   * the assign_certificate_ip script, allowing the IP to be used
   * by other certificates.
   *
   * @deprecated this is now based on the site URI
   * @see free_certificate_site()
   */
  static function free_certificate_ip($ssl_key, $server) {
    return FALSE;
  }


  /**
   * Retrieve the status of a certificate on this server.
   *
   * This is primarily used to know when it's ok to remove the file.
   * Each time a config file uses the key on the server, it touches
   * a 'receipt' file, and every time the site stops using it,
   * the receipt is removed.
   *
   * This function just checks if any of the files are still present.
   */
  static function certificate_in_use($ssl_key, $server) {
    $pattern = $server->http_ssld_path . "/$ssl_key/*.receipt";
    return sizeof(glob($pattern));
  }


  /**
   * Check for an existing record for this IP address.
   *
   * @deprecated we only use the URI-based allocation now
   */
  static function get_ip_certificate($ip, $server) {
    return FALSE;
  }

  /**
   * Verify server.
   */
  function verify() {
    if ($this->context->type === 'server') {
      provision_file()->create_dir($this->server->ssld_path, dt("Central SSL certificate repository."), 0700);

      provision_file()->create_dir($this->server->http_ssld_path,
        dt("SSL certificate repository for %server",
        array('%server' => $this->server->remote_host)), 0700);

      $this->sync($this->server->http_ssld_path, array(
        'exclude' => $this->server->http_ssld_path . '/*',  // Make sure remote directory is created
      ));
    }

    // Call the parent at the end. it will restart the server when it finishes.
    parent::verify();
  }
}

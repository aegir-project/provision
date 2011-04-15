<?php
// $Id$

/**
 * @file The base implementation of the SSL capabale web service.
 */

/**
 * The base class for SSL supporting servers.
 *
 * In general, these function the same as normal servers, but have an extra
 * port and some extra variables in their templates.
 */
class provisionService_http_ssl extends provisionService_http_public {
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
    $this->context->setProperty('ssl_key', null);
  }


  function config_data($config = null, $class = null) {
    $data = parent::config_data($config, $class);
    $data['http_ssl_port'] = $this->server->http_ssl_port;

    if ($config == 'site' && $this->context->ssl_enabled) {
      
      if ($this->context->ssl_enabled == 2) {
        $data['ssl_redirection'] = TRUE;
        $data['redirect_url'] = "https://{$this->context->uri}";
      }

      if ($ssl_key = $this->context->ssl_key) {
        // Retrieve the paths to the cert and key files.
        // they are generated if not found.

        $certs = $this->get_certificates($ssl_key);
        $data = array_merge($data, $certs);

        // assign ip address based on ssl_key
        $ip = provisionService_http_ssl::assign_certificate_ip($ssl_key, $this->server);

        if (!$ip) {
          drush_set_error("SSL_IP_FAILURE", dt("There are no more IP addresses available on %server for the %ssl_key certificate.", array(
            "%server" => $this->server->remote_host,
            "%ssl_key" => $ssl_key,
          )));
        }
        else {
          $data['ip_address'] = $ip;

        }
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

    return $certs;
  }

  /**
   * Generate a self-signed certificate for that key.
   *
   * Because we only generate certificates for sites we make
   * some assumptions based on the uri, but this cert should
   * REALLY be replaced by the admin as soon as possible.
   */
  function generate_certificates($ssl_key) {
    $path = "{$this->server->ssld_path}/{$ssl_key}";

    provision_file()->create_dir($path,
      dt("SSL certificate directory for %ssl_key", array(
        '%ssl_key' => $ssl_key
      )), 0700);

    if (provision_file()->exists($path)->status()) {
      $pass = 'pass';

      // generate a key
      drush_shell_exec('openssl genrsa -passout pass:%s -des3 -out %s/openssl.key.orig 1024', $pass, $path);

      // unsign it
      drush_shell_exec('openssl rsa -passin pass:%s -in %s/openssl.key.orig -out %s/openssl.key', $pass, $path, $path);

      // Generate the CSR
      $ident = "/C=us/CN={$this->context->uri}/OU={$this->context->uri}/emailAddress=admin@{$this->context->uri}";
      drush_shell_exec("openssl req -new -subj '%s' -key %s/openssl.key -out %s/openssl.csr -batch", $ident, $path, $path);

      drush_shell_exec("openssl x509 -req -days 365 -in %s/openssl.csr -signkey %s/openssl.key  -out %s/openssl.crt", $path, $path, $path);
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
   */
  static function assign_certificate_ip($ssl_key, $server) {
    $path = $server->http_ssld_path;

    $pattern = "{$path}/{$ssl_key}__*.receipt";
    $files = glob($pattern);
    if (sizeof($files) == 1) {
      $pattern = "/^{$ssl_key}__(.*)\.receipt$/";
      preg_match($pattern, basename($files[0]), $matches);
      if (in_array($matches[1], $server->ip_addresses)) {
        // Return an existing match.
        return $matches[1];
      }

      // This is a stale match, remove it.
      // Any sites using it will either find a new
      // IP on the next verify task, or fail.
      unlink($files[0]);
    }

    // try to assign one
    foreach ($server->ip_addresses as $ip) {
      if (!provisionService_http_ssl::get_ip_certificate($ip, $server)) {
        touch("{$path}/{$ssl_key}__{$ip}.receipt");
        return $ip;
      }
    }

    return FALSE; // generate error
  }


  /**
   * Remove the certificate's lock on the server's public IP.
   *
   * This function will delete the receipt file left behind by
   * the assign_certificate_ip script, allowing the IP to be used
   * by other certificates.
   */
  static function free_certificate_ip($ssl_key, $server) {
    $ip = provisionService_http_ssl::assign_certificate_ip($ssl_key, $server);
    $file = "{$server->http_ssld_path}/{$ssl_key}__{$ip}.receipt";
    if (file_exists($file)) {
      unlink($file);
    }
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
   */
  static function get_ip_certificate($ip, $server) {
    $path = $server->http_ssld_path;

    $pattern = "{$path}/*__{$ip}.receipt";
    $files = glob($pattern);
    if (sizeof($files) == 1) {
      $pattern = "/^(.*)__{$ip}\.receipt$/";
      preg_match($pattern, basename($files[0]), $matches);
      return $matches[1];
    }

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


/**
 * Base class for SSL enabled server level config.
 */
class provisionConfig_http_ssl_server extends provisionConfig_http_server {
  public $template = 'server_ssl.tpl.php';
  public $description = 'encryption enabled webserver configuration';
}


/**
 * Base class for SSL enabled virtual hosts.
 *
 * This class primarily abstracts the process of making sure the relevant keys
 * are synched to the server when the config files that use them get created.
 */
class provisionConfig_http_ssl_site extends provisionConfig_http_site {
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
        $this->data['ssl_cert']);
      provision_file()->copy(
        $this->data['ssl_cert_key_source'],
        $this->data['ssl_cert_key']);

      // Sync the key directory to the remote server.
      $this->data['server']->sync($path, array(
       'exclude' => "{$path}/*.receipt",  // Don't need to synch the receipts
     ));
    }
    elseif ($ip = $ip_addresses[$this->data['server']->name]) {
      if ($ssl_key = provisionService_http_ssl::get_ip_certificate($ip, $this->data['server'])) {
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
      if ($ssl_key = provisionService_http_ssl::get_ip_certificate($ip, $this->data['server'])) {
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

    $used = provisionService_http_ssl::certificate_in_use($ssl_key, $this->data['server']);

    if (!$used) {
      // we can remove the certificate from the server ssl.d directory.
      _provision_recursive_delete($path);
      // remove the file from the remote server too.
      $this->data['server']->sync($path);

      // Most importantly, we remove the hold this cert had on the IP address.
      provisionService_http_ssl::free_certificate_ip($ssl_key, $this->data['server']);
    }
  }


}


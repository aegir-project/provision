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

  function init() {
    parent::init();

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


  function config_data($config = null, $class = null) {
    $data = parent::config_data($config, $class);

    if ($config == 'site' && $this->context->ssl_enabled) {
      if ($ssl_key = $this->context->ssl_key) {
        // Retrieve the paths to the cert and key files.
        // they are generated if not found.
        $certs = $this->get_certificates($ssl_key);
        $data = array_merge($data, $certs);

        // assign ip address based on ssl_key
        $data['ip_address'] = $this->server->ip_addresses[0];
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
      $exists = provision_file()->exists($cert).status();
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

    $created = provision_file()->create_dir($path,
      dt("SSL certificate directory for %ssl_key", array(
        '%ssl_key' => $ssl_key
      )), 0700)->status();

    if ($created) {
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
   * Retrieve the status of a certificate on this server.
   *
   * This is primarily used to know when it's ok to remove the file.
   */
  static function certificate_in_use($ssl_key) {
    // TODO - status checking
    return true;
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
 * Base class for SSL enabled virtual hosts.
 *
 * This class primarily abstracts the process of making sure the relevant keys
 * are synched to the server when the config files that use them get created.
 */
class provisionConfig_http_ssl_site extends provisionConfig_http_site {
  public $template = 'vhost_ssl.tpl.php';
  public $disabled_template = 'vhost_ssl_disabled.tpl.php';


  function write() {
    parent::write();

    // Make sure the ssl.d directory in the server ssl.d exists. 
    provision_file()->create_dir(dirname($this->data['ssl_cert']), 
    dt("SSL Certificate directory for %key on %server", array(
      '%key' => $this->ssl_key,
      '%server' => $this->data['server']->remote_host,
    )), 700);

    // Copy the certificates to the server's ssl.d directory.
    provision_file()->copy(
      $this->data['ssl_cert_source'],
      $this->data['ssl_cert']);
    provision_file()->copy(
      $this->data['ssl_cert_key_source'],
      $this->data['ssl_cert_key']);

    // Sync the key directory to the remote server.
    $this->data['server']->sync(dirname($this->data['ssl_cert']));
  }

  /**
   * Remove a stale certificate file from the server.
   */
  function unlink() {
    parent::unlink();

    $used = provisionService_http_ssl::certificate_in_use($this->data['ssl_key']);
    if (!$used) {
      // we can remove the certificate from the server ssl.d directory.
      provision_file()->rmdir(dirname($this->data['ssl_cert']));
      // remove the file from the remote server too.
      $this->data['server']->sync(dirname($this->data['ssl_cert']));
    }
  }

  function __construct($owner, $data = array()) {
    parent::__construct($owner, $data);
  
    if (!$this->ssl_enabled || !$this->ssl_key) {
      throw new provisionException_continue("SSL has not been enabled for this site. SSL vhost not necessary"); 
    }
  }

  function filename() {
    $this->data['http_vhostd_path'] . '/' . $this->uri . '_ssl';
  }
}


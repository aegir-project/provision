<?php
/**
 * @file
 * The base Provision DNS service.
 */

class Provision_Service_dns extends Provision_Service {
  public $service = 'dns';
  public $slave = NULL;

  /**
   * Helper function to increment a zone's serial number.
   *
   * @param $serial
   *    A serial in YYYYMMDDnn format. If NULL, a new serial based on
   *    the date will be generated.
   *
   * @return
   *    The serial, incremented based on current date and index
   */
  static public function increment_serial($serial = NULL) {
    $today = date('Ymd');
    if (is_null($serial)) {
      return $today . '00';
    }
    $date = substr($serial, 0, 8); # Get the YYYYMMDD part
    if ($date != $today) {
      return $today . '00';
    }
    else {
      $index = substr($serial, 8, 2); # Get the index part
      if ($index >= 99) {
        drush_set_error("serial number overflow");
      }
      else {
        $index++;
      }
      return $date . sprintf('%02d', $index);
    }
  }


  function parse_configs() {
    return $this->_each_server("parse_configs");
  }

  function init_server() {
    parent::init_server();

    // Path for storing data store config files.
    $this->server->dns_data_path = $this->server->aegir_root . '/config/dns.d';

    if (!is_null($this->application_name)) {
      $app_dir = "{$this->server->config_path}/{$this->application_name}";
      $this->server->dns_app_path = $app_dir;
      $this->server->dns_zoned_path = "{$app_dir}/zone.d";
      $this->server->dns_hostd_path = "{$app_dir}/host.d";
    }

    $this->server->setProperty('slave_servers', array());
    $this->server->setProperty('dns_default_mx', NULL); # XXX: until we get full zone management
    $this->server->setProperty('dns_ttl', 86400); # 24h
    $this->server->setProperty('dns_refresh', 21600); # 6h
    $this->server->setProperty('dns_retry', 3600); # 1h
    $this->server->setProperty('dns_expire', 604800); # 7d
    $this->server->setProperty('dns_negativettl', 86400); # 24h
  }

  function init_site() {
    parent::init_site();

    $this->context->setProperty('dns_zone', NULL);
    if (is_null($this->context->dns_zone)) {
      $this->context->dns_zone = $this->guess_zone($this->context->uri);
    }

    $this->context->dns_zone_subdomain = trim(str_replace($this->context->dns_zone, '', $this->context->uri), '.');
  }

  /**
   * Run a method on each slave server
   *
   * This function does a logical AND on the return status of each of the
   * methods, and returns TRUE only if they all returned something that
   * can be interpreted as TRUE.
   *
   * @todo this is a duplicate of the cluster function of the same name, they
   * need to be merged, but then the cluster_web_server parameter need to be
   * renamed...
   *
   * @see Provision_Service_http_cluster::_each_server()
   */
  function _each_server($method, $args = array()) {
    // Return True by default.
    $ret = TRUE;
    foreach ($this->server->slave_servers as $server) {
      // If any methods return false, return false for the whole operation.
      $result = call_user_func_array(array(d($server)->service($this->service, $this->context), $method), $args);
      $ret = $ret && $result;
    }
    return $ret;
  }

  function verify_server_cmd() {
    provision_file()->create_dir($this->server->dns_data_path, dt("DNS data store"), 0700);

    if (!is_null($this->application_name)) {
      // Ensure that the base DNS configuration folder is at least permissive
      // for users other than the owner, sub folders and files can further
      // restrict access normally.
      provision_file()->create_dir($this->server->dns_app_path, dt("DNS pre-configuration"), 0711);

      provision_file()->create_dir($this->server->dns_zoned_path, dt("DNS zone configuration"), 0755);
      $this->sync($this->server->dns_zoned_path, array(
        'exclude' => $this->server->dns_zoned_path . '/*',  // Make sure remote directory is created
      ));

      provision_file()->create_dir($this->server->dns_hostd_path , dt("DNS host configuration"), 0755);
      $this->sync($this->server->dns_hostd_path, array(
        'exclude' => $this->server->dns_hostd_path . '/*',  // Make sure remote directory is created
      ));

      // TODO: create a slave zone path too.

      $this->create_config('server');
    }

  }

  function config_data($config = NULL, $class = NULL) {
    $data = parent::config_data($config, $class);
    if (!is_null($this->application_name)) {
      $data['dns_data_path'] = $this->server->dns_zoned_path;
      $data['dns_zoned_path'] = $this->server->dns_zoned_path;
      $data['dns_hostd_path'] = $this->server->dns_hostd_path;
    }

    if ($config == 'host') {
      // get the IP explicitely allocate to this site
      $ips = drush_get_option('ip_address', array(), 'site');
      // .. or the server IPs if none is allocated
      if (count($ips) < 1) {
        $ips = $this->server->ip_addresses;
      }
      $data['ip_address'] = $ips;
    }

    return $data;
  }


/**
 * Guess in which zone we should create the record
 *
 * This function will examine the existing zones to find to which
 * this host belongs to.
 *
 * @param $host the name of the record to add (e.g. www.example.com)
 *
 * @returns array the record and zone name to add the record to (e.g. www and example.com)
 */
  function guess_zone($host, $return = 'tld') {
    static $zone_cache;

    if (!isset($zone_cache[$host])) {
      $tld = $host;
      $zones = $this->config('server')->record_get();

      $parts = explode(".", $host);
      $subdomain = array();
      $found = FALSE;
      while (!$found && (count($parts) > 2)) {
        $tld = join(".", $parts);
        if (isset($zones[$tld])) {
          $found = TRUE;
        }
        else {
          $scrap = array_shift($parts);
          $subdomain[] = $scrap;
          drush_log("zone $tld not found, ditching $scrap, count: " . count($parts));
          $found = FALSE;
        }
      }

      // this is necessary if we hit the limit of two subdomains
      $tld = join(".", $parts);
      $subdomain = join(".", $subdomain);

      $zone_cache[$host] = array('tld' => $tld, 'subdomain' => $subdomain);
    }
    else {
      $tld = $zone_cache[$host]['tld'];
      $subdomain = $zone_cache[$host]['subdomain'];
    }


    drush_log("guess_zone guessed parts $tld, $subdomain");

    if ($return == 'subdomain') {
      if (empty($subdomain)) {
        return '@';
      }
      else {
        return $subdomain;
      }
    }

    return $tld;
  }

  /**
   * This creates a zone, which mostly consists of adding the SOA record.
   */
  function create_zone($zone = NULL) {
    if (is_null($zone) && ($this->context->type == 'site')) {
      $host = $this->context->uri;
      $zone = $this->context->dns_zone;
      $sub = $this->context->dns_zone_subdomain;
    }
    if (empty($zone)) {
      return drush_set_error('DRUSH_DNS_NO_ZONE', "Could not determine the zone to create");
    }

    drush_log(dt("creating zone %zone", array("%zone" => $zone)));
    $status = $this->config('zone', $zone)->write();

    if ($status) {
      drush_log(dt("recording zone in server configuration"));
      $status = $this->config('server')->record_set($zone, $zone)->write();
    }

    if ($status) {
      drush_log(dt("creating zone configuration on slaves"));
      $status = $this->_each_server("create_zone", array($zone));
    }
    return $status;
  }

  /**
   * This completely drops a zone, without any checks.
   */
  function delete_zone($zone) {
    $status = $this->config('zone', $zone)->unlink();
    $status = $status && $this->config('server')->record_del($zone, $zone)->write();

    if ($status) {
      drush_log(dt("deleting zone configuration from slaves"));
      $status = $this->_each_server("delete_zone", array($zone));
    }
    return $status;
  }

  /**
   * Create a host in DNS.
   *
   * This can do a lot of things, create a zonefile, add a record to a
   * zonefile, it's going to make its best guess doing the Right
   * Thing.
   *
   * @arg $host string the hostname to create. If NULL, we look in the
   * current context (should be a site) for a URI.
   */
  function create_host($host = NULL) {
    if (!is_null($host)) {
      $zone = $this->guess_zone($host);
      $sub = $this->guess_zone($host, 'subdomain');
      $aliases = array();
    }
    elseif ($this->context->type == 'site') {
      $host = $this->context->uri;
      $zone = $this->context->dns_zone;
      $sub = $this->context->dns_zone_subdomain;
      $aliases = $this->context->aliases;
    }
    else {
      return drush_set_error('DRUSH_DNS_NO_ZONE', "Could not determine the zone to create");
    }

    $ips = drush_get_option('ip_address', array(), 'site');

    if (!$ips && count($ips) < 1) {
      drush_log(dt("no IP found for server, trying loopback"));
      $ips = array('127.0.0.1');
    }

    $this->config('zone', $zone)->record_set($sub, array('A' => $ips));
    foreach ($aliases as $alias) {
      if ($this->guess_zone($alias) == $zone) {
        $this->config('zone', $zone)->record_set($this->guess_zone($alias, 'subdomain'),
          array('CNAME' => array($zone . '.')));
      }
    }

    $this->create_zone($zone);
  }


  /**
   * Delete a host from DNS
   *
   * Similar to create host, this will seek and destroy that host throughout zonefiles.
   *
   * @arg $host string the hostname to create. If NULL, we look in the
   * current context (should be a site) for a URI.
   */
  function delete_host($host = NULL) {
    if (!is_null($host)) {
      $zone = $this->guess_zone($host);
      $sub = $this->guess_zone($host, 'subdomain');
      $aliases = array();
    }
    elseif ($this->context->type == 'site') {
      $host = $this->context->uri;
      $zone = $this->context->dns_zone;
      $sub = $this->context->dns_zone_subdomain;
      $aliases = $this->context->aliases;
    }
    else {
      return drush_set_error('DRUSH_DNS_NO_ZONE', "Could not determine the zone to create");
    }

    // remove the records from the zone store
    $this->config('zone', $zone)->record_set($sub, array('A' => NULL));
    foreach ($aliases as $alias) {
      if ( $this->guess_zone($alias) == $zone) {
        $this->config('zone', $zone)->record_set($this->guess_zone($alias, 'subdomain'),
          array('CNAME' => NULL));
      }
    }
    $this->config('zone', $zone)->write();
  }

}

<?php

/**
 * Implementation of a slave DNS service through BIND9
 *
 * A lot of this is inspired by the BIND implementation of the DNS service and
 * the cluster HTTP service.
 */
class provisionService_dns_bind_slave extends provisionService_dns {
  protected $application_name = 'bind';

  protected $has_restart_cmd = TRUE;
  
  function default_restart_cmd() {
    return provisionService_dns_bind::bind_default_restart_cmd();
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'provisionConfig_bind_slave';
  }

  function parse_configs() {
    $this->restart();
  }

  function verify_server_cmd() {
    if (!is_null($this->application_name)) {
      provision_file()->create_dir($this->server->dns_zoned_path, dt("DNS slave zone configuration"), 0775);
      $this->sync($this->server->dns_zoned_path, array(
        'exclude' => $this->server->dns_zoned_path . '/*',  // Make sure remote directory is created
      ));

      $this->create_config('server');
    }

  }

  /**
   * Create the zonefile record on the slave server
   *
   * Contrarily to the parent class implementation, this *only* creates the
   * bind config (managed through the provisionConfig_bind_slave class), and no
   * zonefile, because the zonefile should be managed by bind itself through
   * regular zone transfers.
   *
   * Note that this function shouldn't be called directly through the API, but
   * only from the master server's create_zone() function.
   *
   * @arg $zone string the zonefile name to create
   *
   * @see provisionService_dns::create_zone()
   */
  function create_zone($zone = null) {
    if (is_null($zone) && ($this->context->type == 'site')) {
      $host = $this->context->uri;
      $zone = $this->context->dns_zone;
      $sub = $this->context->dns_zone_subdomain;
    }
    if (empty($zone)) {
      return drush_set_error('DRUSH_DNS_NO_ZONE', "Could not determine the zone to create");
    }

    drush_log(dt("recording zone in slave configuration"));
    $status = $this->config('server')->record_set($zone, $zone)->write();

    return $status;
  }

  /**
   * This completely drops a zone, without any checks.
   */
  function delete_zone($zone) {
    return $this->config('server')->record_del($zone, $zone)->write();
  }
}

class provisionConfig_bind_slave extends provisionConfig_dns_server {
  public $template = 'slave.tpl.php';

  function process() {
    parent::process();
    if ($this->context->type == 'server') {
     $ips = $this->context->ip_addresses;
    }
    else {
     $ips = $this->context->server->ip_addresses;
    }
    $this->data['master_ip_list'] = implode(';', $ips);
  }
}

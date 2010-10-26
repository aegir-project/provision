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

  /**
   * Create the zonefile record on the slave server
   *
   * This differs from the parent implementation because it *requires* you to
   * pass the master server, as it is necessary for the slave to know what its
   * slave is (and it can vary according to the zone).
   *
   * Contrarily to the parent class implementation, this *only* creates the
   * bind config (managed through the provisionConfig_bind_slave class), and no
   * zonefile, because the zonefile should be managed by bind itself through
   * regular zone transfers.
   *
   * Note that this function shouldn't be called directly through the API, but
   * only from the master server's create_zone() function.
   *
   * @todo this needs to be pushed up in a provisionService_dns_slave common
   * class
   * 
   * @arg $master mixed a string or array of strings of IPs pointing to the
   * master server for this zone
   *
   * @arg $zone string the zonefile name to create
   *
   * @see provisionService_dns::create_zone()
   */
  function create_zone($master, $zone = null) {
    if (is_null($zone) && ($this->context->type == 'site')) {
      $host = $this->context->uri;
      $zone = $this->context->dns_zone;
      $sub = $this->context->dns_zone_subdomain;
    }
    if (empty($zone)) {
      return drush_set_error('DRUSH_DNS_NO_ZONE', "Could not determine the zone to create");
    }

    drush_log(dt("recording zone in slave configuration"));
    $this->config('server')->record_set($zone, $master)->write();
  }

}

class provisionConfig_bind_slave extends provisionConfig_dns_server {
  public $template = 'slave.tpl.php';
}

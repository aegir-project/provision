<?php

/**
 * Implementation of a slave DNS service through BIND9
 *
 * A lot of this is inspired by the BIND implementation of the DNS service and
 * the cluster HTTP service.
 */
class provisionService_dns_bind_slave extends provisionService_dns {
  protected $application_name = 'bind_slave';

  protected $has_restart_cmd = TRUE;
  
  function default_restart_cmd() {
    return provisionService_dns_bind::bind_default_restart_cmd();
  }

  function init_server() {
    parent::init_server();
    $this->server->setProperty('dns_master_server', null);
    $this->configs['server'][] = 'provisionConfig_bind_slave';
  }

  function parse_configs() {
    $this->restart();
  }

  function create_zone($zone = null) {
    if (is_null($zone) && ($this->context->type == 'site')) {
      $host = $this->context->uri;
      $zone = $this->context->dns_zone;
      $sub = $this->context->dns_zone_subdomain;
    }
    if (empty($zone)) {
      return drush_set_error('DRUSH_DNS_NO_ZONE', "Could not determine the zone to create");
    }

    drush_log(dt("recording zone in server configuration"));
    $this->config('server')->record_set($zone, $zone)->write();
  }

}

class provisionConfig_bind_slave extends provisionConfig_dns_server {
  public $template = 'slave.tpl.php';
}

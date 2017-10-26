<?php
/**
 * @file
 * A context's subscription to a service. Handles properties specific to a
 * context for each service.
 *
 * @see Provision_Service
 */

namespace Aegir\Provision;

class ServiceSubscription {
  
   public $context;
   public $service;
   public $server;
   public $type;
  
  function __construct($context, $server, $service_name) {
      $this->context = $context;
      $this->server = $server;
      $this->service = $server->getService($service_name);
      $this->type = $server->getService($service_name)->type;
  }
  
  public function verify() {
      $this->service->verifySubscription($this);
  }
}

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
   public $properties = [];
  
  function __construct($context, $server, $service_name) {
      $this->context = $context;
      $this->server = $server;
      $this->service = $server->getService($service_name);
      $this->type = $server->getService($service_name)->type;
      
      if (isset($context->config['service_subscriptions'][$service_name]['properties'])) {
          $this->properties = $context->config['service_subscriptions'][$service_name]['properties'];
      }
  }
  
  public function verify() {
      $this->service->verifySubscription($this);
  }
}

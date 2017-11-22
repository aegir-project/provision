<?php
/**
 * @file
 * A context's subscription to a service. Handles properties specific to a
 * context for each service.
 *
 * @see Provision_Service
 */

namespace Aegir\Provision;

use Aegir\Provision\Common\ContextAwareTrait;
use Aegir\Provision\Common\ProvisionAwareTrait;

class ServiceSubscription {
  
   public $context;
   public $service;
   public $server;
   public $type;
   public $properties = [];
   
   use ProvisionAwareTrait;
   use ContextAwareTrait;
  
  function __construct(
      Context $context,
      $server,
      $service_name
  ) {
      $this->setContext($context);
      $this->server = Provision::getContext($server, $context->getProvision());
      $this->service = $this->server->getService($service_name);
      $this->type = $this->server->getService($service_name)->type;
      
      if (isset($context->config['service_subscriptions'][$service_name]['properties'])) {
          $this->properties = $context->config['service_subscriptions'][$service_name]['properties'];
      }
      
      $this->setProvision($context->getProvision());
  }
  
  public function verify() {
      return $this->service->verifySubscription($this);
  }
  
  public function getFriendlyName() {
      return $this->service->getFriendlyName();
  }


    /**
     * Get a specific property.
     *
     * @param $name
     * @return mixed
     * @throws \Exception
     */
    public function getProperty($name) {
        if (isset($this->properties[$name])) {
            return $this->properties[$name];
        }
        else {
            throw new \Exception("Property '$name' on Service Subscription does not exist.");
        }
    }
}

<?php
/**
 * @file
 * The base Provision service class.
 *
 * @see Provision_Service
 */

namespace Aegir\Provision;

class Service
{
    
    public $type;
    
    public $properties;
    
    /**
     * @var Context;
     * The context in which this service stores its data
     *
     * This is usually an object made from a class derived from the
     * Provision_Context base class
     *
     * @see Provision_Context
     */
    public $context;
    
    /**
     * @var string
     * The machine name of the service.  ie. http, db
     */
    const SERVICE = 'service';
    
    /**
     * @var string
     * A descriptive name of the service.  ie. Web Server
     */
    const SERVICE_NAME = 'Service Name';
    
    function __construct($service_config, $context)
    {
        $this->context = $context;
        $this->type = $service_config['type'];
        $this->properties = $service_config['properties'];
    }
    
    /**
     * React to the `provision verify` command.
     */
    function verify()
    {
        $this->writeConfigurations();
    }
    
    /**
     * React to the `provision verify` command.
     */
    function verifySubscription(ServiceSubscription $serviceSubscription)
    {
        $this->writeConfigurations($serviceSubscription);
    }
    
    /**
     * List context types that are allowed to subscribe to this service.
     *
     * @return array
     */
    static function allowedContexts()
    {
        return [];
    }
    
    /**
     * Write this service's configurations.
     */
    protected function writeConfigurations()
    {
        if (empty($this->getConfigurations()[$this->context->type])) {
            return;
        }
        $this->context->application->logger->info(
            'CONTEXT '.$this->context->type
        );
        foreach (
            $this->getConfigurations()[$this->context->type] as
            $configuration_class
        ) {
            $config = new $configuration_class($this->context, $this);
            $config->write();
            $this->context->application->logger->info(
                'Wrote '.$config->description.' to '.$config->filename()
            );
        }
    }
    
    /**
     * Stub for this services configurations.
     */
    protected function getConfigurations()
    {
        return [];
    }
    
    /**
     * Return the SERVICE_TYPE
     *
     * @return mixed
     */
    public function getType()
    {
        return $this::SERVICE_TYPE;
    }
    
    /**
     * Return the SERVICE_TYPE
     *
     * @return mixed
     */
    public function getName()
    {
        return $this::SERVICE;
    }
    
    /**
     * Return a list of user configurable options that this service provides to Server Context objects.
     */
    static function server_options()
    {
        return [];
        //        return [
        //            'http_port' => 'The port which the web service is running on.',
        //            'web_group' => 'server with http: OS group for permissions; working default will be attempted',
        //        ];
    }
    
    /**
     * Return a list of user configurable options that this service provides to Platform Context objects.
     */
    static function platform_options()
    {
        return [];
        //        return [
        //            'platform_extra_config' => 'Extra lines of configuration to add to this platform.',
        //        ];
    }
    
    /**
     * Return a list of user configurable options that this service provides to Site Context objects.
     */
    static function site_options()
    {
        return [];
        //      return [
        //          'site_mail' => 'The email address to use for the ServerAdmin configuration.',
        //      ];
    }
    
}
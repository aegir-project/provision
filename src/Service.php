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
     * The context that provides this service.
     *
     * @see \Aegir\Provision\Context
     */
    public $provider;
    
    /**
     * @var \Aegir\Provision\Application;
     */
    public $application;
    
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
    
    function __construct($service_config, $provider_context)
    {
        $this->provider = $provider_context;
        $this->application = $provider_context->application;
        $this->type = $service_config['type'];
        $this->properties = $service_config['properties'];
    }
    
    /**
     * Retrieve the class name of a specific service type.
     *
     * @param $service
     *   The service requested. Typically http, db.
     *
     * @param $type
     *   The type of service requested. For example: apache, nginx, mysql.
     *
     * @return string
     */
    static function getClassName($service, $type = NULL) {
        $service = ucfirst($service);
        $type = ucfirst($type);
        
        if ($type) {
            return "\Aegir\Provision\Service\\{$service}\\{$service}{$type}Service";
        }
        else {
            return "\Aegir\Provision\Service\\{$service}Service";
        }
    }
    
    /**
     * React to the `provision verify` command.
     */
    function verify()
    {
        return [
            'configuration' => $this->writeConfigurations(),
            'service' => $this->restartService(),
        ];
    }

    /**
     * React to `provision verify` command when run on a subscriber, to verify the service's provider.
     *
     * This is used to allow skipping of the service restart.
     */
    function verifyProvider()
    {
        return [
            'configuration' => $this->writeConfigurations(),
        ];
    }

    /**
     * Run the services "restart_command".
     * @return bool
     */
    protected function restartService() {
        if (empty($this->properties['restart_command'])) {
            return TRUE;
        }
        else {
            try {
                $this->application->provision->getOutput()->exec($this->properties['restart_command'], 'Restarting service...');
                $this->application->io->successLite('Service restarted.');
                sleep(1);
                return TRUE;
            }
            catch (\Exception $e) {
                $this->application->io->errorLite('Unable to restart service: ' . $e->getMessage());
            }
        }
        return FALSE;
    }

    /**
     * React to the `provision verify` command.
     */
    function verifySubscription(ServiceSubscription $serviceSubscription)
    {
        return [
            'configuration' => $this->writeConfigurations($serviceSubscription),
        ];
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
     *
     * @param \Aegir\Provision\ServiceSubscription|null $serviceSubscription
     *
     * @return bool
     */
    protected function writeConfigurations(ServiceSubscription $serviceSubscription = NULL)
    {
        // If we are writing for a serviceSubscription, use the provider context.
        if ($serviceSubscription) {
            $context = $serviceSubscription->context;
        }
        else {
            $context = $this->provider;
        }
        
        if (empty($this->getConfigurations()[$context->type])) {
            return TRUE;
        }
        
        $success = TRUE;
        foreach ($this->getConfigurations()[$context->type] as $configuration_class) {
            try {
                $config = new $configuration_class($context, $this);
                $config->write();
                $context->application->io->successLite(
                    'Wrote '.$config->description.' to '.$config->filename()
                );
            }
            catch (\Exception $e) {
                $context->application->io->errorLite(
                    'Unable to write '.$config->description.' to '.$config->filename() . ': ' . $e->getMessage()
                );
                $success = FALSE;
            }
        }
        return $success;
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
            throw new \Exception("Property '$name' on service '$this->type' does not exist.");
        }
    }
    
    /**
     * Return the SERVICE_TYPE
     *
     * @return mixed
     */
    public function getFriendlyName()
    {
        return $this::SERVICE_NAME;
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
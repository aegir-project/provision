<?php
/**
 * @file
 * The base Provision service class.
 *
 * @see Provision_Service
 */

namespace Aegir\Provision;

use Aegir\Provision\Common\ContextAwareTrait;
use Aegir\Provision\Common\ProvisionAwareTrait;
use Aegir\Provision\Context\ServerContext;
use Psr\Log\LoggerAwareTrait;
use Robo\Common\BuilderAwareTrait;
use Robo\Common\OutputAdapter;
use Robo\Contract\BuilderAwareInterface;

class Service implements BuilderAwareInterface
{
    use BuilderAwareTrait;
    use ProvisionAwareTrait;
    use ContextAwareTrait;
    use LoggerAwareTrait;

    public $type;
    
    public $properties;
    
    /**
     * @var ServerContext;
     * The context that provides this service.
     *
     * @see \Aegir\Provision\Context
     */
    public $provider;
    
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
    
    function __construct($service_config, Context $provider_context)
    {
        $this->provider = $provider_context;
        $this->setContext($provider_context);
        $this->setProvision($provider_context->getProvision());
        $this->setLogger($provider_context->getProvision()->getLogger());
        
        $this->type = $service_config['type'];
        $this->properties = $service_config['properties'];
        if ($provider_context->getBuilder()) {
            $this->setBuilder($provider_context->getBuilder());
        }
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
     * React to the verify command. Passes off to the method verifySite, verifyServer, verifyPlatform.
     * @return mixed
     */
    public function verify() {
        $method = 'verify' . ucfirst($this->getContext()->type);
        $this->getProvision()->getLogger()->debug("Running method {method} on class {class}", [
            'method' => $method,
            'class' => get_class($this),
        ]);
        return $this::$method();
    }

//
//    /**
//     * React to the `provision verify` command.
//     */
//    function verifySite()
//    {
//        return [
//            'configuration' => $this->writeConfigurations(),
//            'service' => $this->restartService(),
//        ];
//    }
//
//    /**
//     * React to the `provision verify` command.
//     */
//    function verifyPlatform()
//    {
//        return [
//            'configuration' => $this->writeConfigurations(),
//            'service' => $this->restartService(),
//        ];
//    }
//
//    /**
//     * React to `provision verify` command when run on a subscriber, to verify the service's provider.
//     *
//     * This is used to allow skipping of the service restart.
//     */
//    function verifyServer()
//    {
//        return [
//            'configuration' => $this->writeConfigurations(),
//        ];
//    }

    /**
     * Run the services "restart_command".
     * @return bool
     */
    protected function restartService() {
        if (empty($this->getProperty('restart_command'))) {
            throw new \Exception('Unable to restart service: There is no restart_command specified.');
        }
        else {

            try {
                $this->provider->shell_exec($this->getProperty('restart_command'));
                return TRUE;
            }
            catch (\Exception $e) {
                throw new \Exception('Unable to restart service: ' . $e->getMessage());
            }
        }
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
    protected function writeConfigurations(Context $context = NULL)
    {
        // If we are writing for a serviceSubscription, use the provider context.
        if ($context == NULL) {
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
                $context->getProvision()->getLogger()->info(
                    'Wrote {description} to {path}.', [
                        'description' => $config->description,
                        'path' => $config->filename(),
                    ]
                );
            }
            catch (\Exception $e) {
                throw new \Exception(strtr(
                    'Unable to write {description} to {path}: {message}', [
                        '{description}' => $config->description,
                        '{path}' => $config->filename(),
                        '{message}' => $e->getMessage(),
                    ]
                ));
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
     * Whether or not this Services has a property.
     *
     * @param $type
     * @return bool
     */
    public function hasProperty($name) {
        if (isset($this->properties[$name])) {
            return TRUE;
        }
        else {
            return FALSE;
        }
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
     * Set a specific property.
     *
     * @param $name
     * @return mixed
     * @throws \Exception
     */
    public function setProperty($name, $value) {
        $this->properties[$name] = $value;
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
    
    
    /**
     * @return \Aegir\Provision\Robo\ProvisionBuilder
     */
    function getBuilder()
    {
        return $this->builder;
    }
    
    /**
     * Load all contexts that subscribe to this provider's service.
     *
     * @return array
     */
    public function getAllSubscribers() {
        $subscribers = [];
        
        foreach ($this->getProvision()->getAllContexts() as $context){
            if (get_class($context) != ServerContext::class) {
                foreach ($context->getSubscriptions() as $subscription) {
                    if ($subscription->server->name == $this->provider->name && $subscription->type == $this->type) {
                        $subscribers[] = $context;
                    }
                }
            }
            
        }
        return $subscribers;
        
    }
}
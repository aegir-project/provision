<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Console\ProvisionStyle;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Aegir\Provision\Property;
use Aegir\Provision\Provision;
use Aegir\Provision\Service;
use Consolidation\AnnotatedCommand\CommandFileDiscovery;
use Symfony\Component\Console\Exception\LogicException;
use Symfony\Component\Console\Input\ArrayInput;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class ServicesCommand
 *
 * @package Aegir\Provision\Command
 */
class ServicesCommand extends Command
{

    /**
     * This command needs a context.
     */
    const CONTEXT_REQUIRED = TRUE;

    /**
     * "list" (default), "add", "remove", or "configure"
     * @var string
     */
    protected $sub_command = '';

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
          ->setName('services')
          ->setDescription('Manage the services attached to servers.')
          ->setHelp(
            'Use this command to add new services to servers, or to add service subscriptions to platforms and sites.'
          )
          ->setDefinition($this->getCommandDefinition())
        ;
    }

    /**
     * Generate the list of options derived from ProvisionContextType classes.
     *
     * @return \Symfony\Component\Console\Input\InputDefinition
     */
    protected function getCommandDefinition()
    {
        $inputDefinition = $this::getCommandOptions();

        $inputDefinition[] = new InputArgument(
          'sub_command',
          InputArgument::OPTIONAL,
          '"list" (default), "add", "remove", or "configure".',
          'list'
        );
        $inputDefinition[] = new InputArgument(
          'service',
          InputArgument::OPTIONAL,
          'http, db, etc.'
        );
        $inputDefinition[] = new InputArgument(
          'server',
          InputArgument::OPTIONAL,
          'The name of the server context to use for this service.'
        );
        $inputDefinition[] = new InputOption(
          'service_type',
          NULL,
          InputOption::VALUE_OPTIONAL,
          'The name of the service type to use.'
        );
        return new InputDefinition($inputDefinition);
    }
    
    /**
     * Load all server_options, site_options, and platform_options from Service classes.
     *
     * Invoked from SaveCommand as well.
     * @return array
     */
    public static function getCommandOptions() {
        $inputDefinition = [];
    
        // Load all service options
        $options = Context::getServiceOptions();

        // For each service type...
        foreach ($options as $service => $service_name) {
        
            // Load every available service type.
            foreach (Context::getServiceTypeOptions($service) as $service_type => $service_name) {
                $class = Service::getClassName($service, $service_type);

                Provision::getProvision()->getLogger()->debug("Loading options from $class $service_type");

                // Load option_documentation() into input options.
                foreach (Context::getContextTypeOptions() as $type => $type_name) {
                    $method = "{$type}_options";
                    foreach ($class::$method() as $option => $description) {
                      $description = "$type_name $service $service_name service: $description";
                      $inputDefinition[] = new InputOption($option, NULL, InputOption::VALUE_OPTIONAL, $description);
                    }
                }
            }
        }
        
        return $inputDefinition;
    }

    /**
     * Validate that the context specified is a server.
     *
     * @param \Symfony\Component\Console\Input\InputInterface $input
     * @param \Symfony\Component\Console\Output\OutputInterface $output
     *
     * @throws \Exception
     */
    protected function initialize(
        InputInterface $input,
        OutputInterface $output
    ) {
//
//        if ($input->getOption('context_name') == 'add') {
//            $this->sub_command = $input->getArgument('context_name');
//            $input->setArgument('context_name', NULL);
//        }
//        else {
//            $this->sub_command = $input->getArgument('sub_command');
//        }
        $this->sub_command = $input->getArgument('sub_command');

        parent::initialize(
            $input,
            $output
        );
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $method = "execute_{$this->sub_command}";
        if (method_exists($this, $method)) {
            $this->$method($input, $output);
        }
    }

    /**
     * List all services attached to this server.
     *
     * @param \Symfony\Component\Console\Input\InputInterface $input
     * @param \Symfony\Component\Console\Output\OutputInterface $output
     */
    protected function execute_list(InputInterface $input, OutputInterface $output) {

        $this->io->comment("List Services");
        $this->context->showServices($this->io);
    }

    /**
     * Add a new service to a server.
     */
    protected function execute_add(InputInterface $input, OutputInterface $output)
    {
        // Ask which service.
        $this->io->comment("Add Services");
        $service = $this->input->getArgument('service')?
            $this->input->getArgument('service'):
            $this->io->choice('Which service?', $this->context->getServiceOptions());
        
        if (empty($service)) {
            throw new \Exception("Argument 'service' must not be empty.");
        }

        // If server, ask which service type.
        if ($this->context->type == 'server') {
            if (empty($this->context->getServiceTypeOptions($service))) {
                throw new \Exception("There was no class found for service $service. Create one named \\Aegir\\Provision\\Service\\{$service}Service");
            }
            
            // Check or ask for service_type option.
            if ($this->input->getOption('service_type')) {
                $service_type = $this->input->getOption('service_type');
                $this->io->comment("Using option service_type=". $service_type);
            }
            else {
                $service_type = $this->io->choice('Which service type?', $this->context->getServiceTypeOptions($service));
            }
    
            // If $service_type is still empty, throw an exception. Happens if using -n
            if (empty($service_type)) {
                throw new \Exception('Option --service_type must be specified.');
            }
            
            // Handle invalid service_type.
            if (!class_exists(Service::getClassName($service, $service_type))) {
                $types = Context::getServiceTypeOptions($service);
                throw new \Exception(strtr("Class not found for service type !type !service. Expecting !class. Check your Class::SERVICE_TYPE constant.", [
                    '!service' => $service,
                    '!type' => $service_type,
                    '!class' => Service::getClassName($service, $service_type),
                ]));
            }

            if ($this->context->hasService($service)) {
                $this->getProvision()->io()->helpBlock("Editing service {$service} provded by server '{$this->context->name}'...", ProvisionStyle::ICON_EDIT);
            }

            // Then ask for all options.
            $properties = $this->askForServiceProperties($service, $service_type);

            $this->io->info("Adding $service service $service_type...");

            $services_key = 'services';
            $service_info = [
                'type' => $service_type,
            ];
        }
        // All other context types are associating with servers that provide the service.
        else {
            if (empty($this->getProvision()->getServerOptions($service))) {
                throw new \Exception("No servers providing $service service were found. Create one with `provision save` or use `provision services` to add to an existing server.");
            }
            
            $server = $this->input->getArgument('server')?
                $this->input->getArgument('server'):
                $this->io->choice('Which server?', $this->getProvision()->getServerOptions($service));

            // Then ask for all options.
            $server_context = $this->getProvision()->getContext($server);
            $properties = $this->askForServiceProperties($service);

            $this->io->info("Using $service service from server $server...");

            $services_key = 'service_subscriptions';
            $service_info = [
                'server' => $server,
            ];
        }

        try {
            $this->context->config[$services_key][$service] = $service_info;
            $this->context->config[$services_key][$service]['properties'] = $properties;

            $this->context->setProperty($services_key, $this->context->config[$services_key]);
            $this->context->save();
            $this->io->success('Service saved to Context!');
        }
        catch (\Exception $e) {
            throw new \Exception("Something went wrong when saving the context: " . $e->getMessage());
        }
    }

    /**
     * Loop through this context type's option_documentation() method and ask for each property.
     *
     * @return array
     */
    private function askForServiceProperties($service, $service_type = NULL) {

        $class = Service::getClassName($service, $service_type);
        $method = "{$this->context->type}_options";

        $options = $class::{$method}();
        $properties = [];
        foreach ($options as $name => $property) {

            // Allows option_documentation to return array of strings for simple properties.
            if ( !$property instanceof Property) {
                $property = Provision::newProperty($property);
            }

            if ($this->context->hasService($service) && $this->context->getService($service)->hasProperty($name) && $this->context->getService($service)->getProperty($name)) {
                $property->default = $this->context->getService($service)->getProperty($name);
            }

            // If option does not exist, ask for it.
            if ($this->input->hasOption($name) && !empty($this->input->getOption($name))) {
                $properties[$name] = $this->input->getOption($name);
                $this->io->comment("Using option {$name}={$properties[$name]}");
            }
            else {
                $properties[$name] = $this->io->ask("{$name} ({$property->description})", $property->default, $property->validate);
            }
        }
        return $properties;
    }
}

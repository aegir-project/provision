<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Consolidation\AnnotatedCommand\CommandFileDiscovery;
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
            'Saves a ProvisionContext object to file. Currently just passes to "drush provision-save".'
          )
          ->setDefinition($this->getCommandDefinition());
    }

    /**
     * Generate the list of options derived from ProvisionContextType classes.
     *
     * @return \Symfony\Component\Console\Input\InputDefinition
     */
    protected function getCommandDefinition()
    {
        $inputDefinition = [];
        $inputDefinition[] = new InputArgument(
          'context_name',
          InputArgument::REQUIRED,
          'Server to work on.'
        );
        $inputDefinition[] = new InputArgument(
          'sub_command',
          InputArgument::OPTIONAL,
          '"list" (default), "add", "remove", or "configure".',
          'list'
        );
        return new InputDefinition($inputDefinition);
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
        parent::initialize(
            $input,
            $output
        );
        if (isset($this->context->type) && $this->context->type != 'server') {
            throw new \Exception('Context must be a server.');
        }

        $this->sub_command = $input->getArgument('sub_command');
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
        $service = $this->io->choice('Which service?', $this->context->getServiceOptions());

        // Then ask which service type
        $service_type = $this->io->choice('Which service type?', $this->context->getServiceTypeOptions($service));

        $this->io->info("Adding $service service $service_type...");

        try {
            $this->context->config['services'][$service] = [
                'type' => $service_type,
            ];
            $this->context->save();
            $this->io->success('Service saved to Context!');
        }
        catch (\Exception $e) {
            throw new \Exception("Something went wrong when saving the context: " . $e->getMessage());
        }
    }
}

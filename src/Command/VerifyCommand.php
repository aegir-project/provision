<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class VerifyCommand
 *
 * Replacement for drush provision-verify command
 *
 * @package Aegir\Provision\Command
 * @see provision.drush.inc
 * @see drush_provision_verify()
 */
class VerifyCommand extends Command
{

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
          ->setName('verify')
          ->setDescription('Verify a Provision Context.')
          ->setHelp(
            'Verify the chosen context: write configuration files, run restart commands, etc. '
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
          'Context to verify'
        );
        return new InputDefinition($inputDefinition);
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $this->io->info('Provision Verify: ' . $this->context_name);

        /**
         * The provision-verify command function looks like:
         *
         *
        function drush_provision_verify() {
            provision_backend_invoke(d()->name, 'provision-save');
            d()->command_invoke('verify');
        }
         */

        $message = $this->context->verify();

        $this->io->comment($message);
    }
}

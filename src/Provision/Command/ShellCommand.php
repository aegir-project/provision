<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Psy\Shell;
use Psy\Configuration;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class ShellCommand
 *
 * @package Aegir\Provision\Command
 */
class ShellCommand extends Command
{
    const CONTEXT_REQUIRED = TRUE;

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
          ->setName('shell')
          ->setDescription($this->trans('commands.shell.description'))
          ->setHelp($this->trans('commands.shell.help'));
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output)
    {

        $process = new \Symfony\Component\Process\Process("bash");
        $process->setTty(TRUE);

        if ($this->context->type == 'site') {
            $dir = $this->context->getProperty('root');
            $process->setCommandLine("cd $dir && bash");
            $this->io->simple("Opening bash shell in " . $dir);

            //@TODO: Allow services to set environment variables for both shell command and virtualhost config.
            $env = $_SERVER;
            $env['db_type'] = $this->context->getSubscription('db')->service->getType();
            $env['db_name'] = $this->context->getSubscription('db')->getProperty('db_name');
            $env['db_user'] = $this->context->getSubscription('db')->getProperty('db_user');
            $env['db_passwd'] = $this->context->getSubscription('db')->getProperty('db_password');
            $env['db_host'] = $this->context->getSubscription('db')->service->provider->getProperty('remote_host');
            $env['db_port'] = $this->context->getSubscription('db')->service->getCreds()['port'];

            $env['PWD'] = $this->context->getProperty('root');

            // If bin dir is found, add to path
            if (file_exists($this->context->getProperty('root') . '/bin')) {
                $env['PATH'] .= ':' . $this->context->getProperty('root') . '/bin';
            }
            if (file_exists($this->context->getProperty('root') . '/vendor/bin')) {
                $env['PATH'] .= ':' . $this->context->getProperty('root') . '/vendor/bin';
            }

            $process->setEnv($env);
        }
        else {
            $this->io->simple("Opening bash shell in " . getcwd());
        }

        $process->run();
    }
}

<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Aegir\Provision\Provision;
use Drupal\Console\Core\Style\DrupalStyle;
use Symfony\Component\Console\Input\ArrayInput;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Input\InputDefinition;
use Symfony\Component\Console\Input\InputOption;
use Symfony\Component\Console\Input\InputArgument;
use Symfony\Component\Console\Output\OutputInterface;

/**
 * Class SetupCommand
 *
 * @package Aegir\Provision\Command
 */
class SetupCommand extends Command
{

    /**
     * {@inheritdoc}
     */
    protected function configure()
    {
        $this
          ->setName('setup')
          ->setDescription('Configure provision for your system.')
          ->setHelp('Provision works by controlling the configuration of the services that power your websites. Run this command to walk through the setup process to get your system ready to host websites.')

        ;
    }

    /**
     * {@inheritdoc}
     */
    protected function execute(InputInterface $input, OutputInterface $output) {

        $console_config_file =  $this->getProvision()->getConfig()->get('console_config_file');

        $this->io->titleBlock('Welcome to the Provision Setup Wizard!');
        $this->io->helpBlock("This command will guide you through configuring your system.");

        // Check for .provision.yml file.
        if (file_exists($this->getProvision()->getConfig()->get('console_config_file'))) {
            $this->io->successLite("Provision CLI configuration file found: <comment>{$console_config_file}</comment>");
        }
        else {
            $this->io->warningLite("Provision CLI configuration file was not found at <comment>{$console_config_file}</comment>");

          if ($this->io->confirm("Would you like to create the file? {$console_config_file}")) {
              $config_path = $this->io->ask('Where would you like Provision to store context and server configuration? (Just hit enter to use the default)', $this->getProvision()->getConfig()->get('config_path'));

              if (!Provision::fs()->isAbsolutePath($config_path)) {
                  $config_path = getcwd() . DIRECTORY_SEPARATOR . $config_path;
              }

              $this->io->successLite("Setting Provision config_path to <comment>{$config_path}</comment>");

              $contexts_path = $config_path . DIRECTORY_SEPARATOR . 'contexts';
              $this->getProvision()->getConfig()->set('config_path', $config_path);
              $this->getProvision()->getConfig()->set('contexts_path', $contexts_path);

              $config = <<<YML
config_path: $config_path
contexts_path: $contexts_path
YML;
              try {
                  Provision::fs()->dumpFile($console_config_file, $config);
              }
              catch (\Exception $e) {
                  throw new \Exception("Unable to create console config file at $console_config_file: " . $e->getMessage());
              }

              $this->io->successLite("Provision CLI configuration written to <comment>{$console_config_file}</comment>");
          }
        }

        // If config_path or contexts_path does not exist...
        if (file_exists($this->getProvision()->getConfig()->get('config_path'))) {
            $this->io->successLite("Config Path <comment>{$this->getProvision()->getConfig()->get('config_path')}</comment> exists.");

            if (file_exists($this->getProvision()->getConfig()->get('contexts_path'))) {
                $this->io->successLite("Contexts Path <comment>{$this->getProvision()->getConfig()->get('contexts_path')}</comment> exists.");
            }
            elseif ($this->io->confirm("Create the 'contexts_path' folder? " . $this->getProvision()->getConfig()->get('contexts_path'))) {
                Provision::fs()->mkdir($this->getProvision()->getConfig()->get('contexts_path'));
                $this->io->successLite("Contexts Path <comment>{$this->getProvision()->getConfig()->get('contexts_path')}</comment> created.");
            }
            else {
                FAIL;
            }
        }
        else {

            // Offer to create the folder for the user.
            if ($this->input->hasParameterOption(array('--no-interaction', '-n'), false) || $this->io->confirm('Should I create the folders ' . $this->getProvision()->getConfig()->get('config_path') . ' and ' . $this->getProvision()->getConfig()->get('contexts_path') . ' ?')) {
                try {

                    Provision::fs()->mkdir($this->getProvision()->getConfig()->get('config_path'), 0700);
                    Provision::fs()->mkdir($this->getProvision()->getConfig()->get('contexts_path'), 0700);

                    $this->io->successLite("Config Path <comment>{$this->getProvision()->getConfig()->get('config_path')}</comment> created.");
                    $this->io->successLite("Contexts Path <comment>{$this->getProvision()->getConfig()->get('contexts_path')}</comment> created.");

                    $this->io->writeln('');
                }
                catch (\Exception $e) {

                    throw new \Exception('Unable to create paths: ' . $e->getMessage());

                }
            }
        }
    }
}

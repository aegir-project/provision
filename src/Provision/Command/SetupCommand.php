<?php

namespace Aegir\Provision\Command;

use Aegir\Provision\Command;
use Aegir\Provision\Context;
use Aegir\Provision\Context\PlatformContext;
use Aegir\Provision\Context\ServerContext;
use Aegir\Provision\Context\SiteContext;
use Aegir\Provision\Provision;
use Consolidation\AnnotatedCommand\ExitCodeInterface;
use Drupal\Console\Core\Style\DrupalStyle;
use Psr\Log\LogLevel;
use Robo\ResultData;
use Symfony\Component\Console\Exception\RuntimeException;
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

        $this->setupConfig();
        $this->setupServer();
    }

    /**
     * Step 1: Check and prepare config files and directories.
     * @throws \Exception
     */
    protected function setupConfig() {
        $console_config_file =  $this->getProvision()->getConfig()->get('console_config_file');

        $this->io->titleBlock('Welcome to the Provision Setup Wizard!');

        $this->io->title('Console Configuration');
        $this->io->block('In this section, we will make sure your Provision CLI configuration and folders are created.');



        // Check for .provision.yml file.
        if (file_exists($this->getProvision()->getConfig()->get('console_config_file'))) {
            $this->io->successLite("Provision CLI configuration file found: <comment>{$console_config_file}</comment>");
        }
        else {
            $this->io->warningLite("Provision CLI configuration file was not found at <comment>{$console_config_file}</comment>");

            $this->io->block('Helpful Tips:');
            $this->io->bulletLite('The <comment>~/.provision.yml</comment> file determines the <comment>config_path</comment> and <comment>contexts_path</comment> settings.');
            $this->io->bulletLite('The <comment>config_path</comment> is where Provision stores server configuration. Each server gets a folder inside this path to store their configuration files, such as Apache virtualhosts. The default config path is <comment>' . $this->getProvision()->getConfig()->get('config_path') . '</comment>');
            $this->io->bulletLite('The <comment>contexts_path</comment> is where Provision stores the metadata about your servers and sites, called <info>Contexts</info>. Contexts are saved as YML files in this folder. The default contexts path is <comment>' . $this->getProvision()->getConfig()->get('contexts_path') . '</comment>');
            $this->io->bulletLite('When Provision asks you <info>a question</info>, it may provide a [<comment>default value</comment>]. If you just hit enter, that default value will be used.');

            $config_path = $this->io->ask('Where would you like Provision to store its configuration?', $this->getProvision()->getConfig()->get('config_path'));
            $contexts_path = $this->io->ask('Where would you like Provision to store its contexts?', $config_path . DIRECTORY_SEPARATOR . 'contexts');

            if ($this->io->confirm("Would you like to create the file {$console_config_file} ?")) {

              if (!Provision::fs()->isAbsolutePath($config_path)) {
                  $config_path = getcwd() . DIRECTORY_SEPARATOR . $config_path;
              }

              $this->io->successLite("Setting Provision config_path to <comment>{$config_path}</comment>");

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
            else {
                $this->io->errorLite("If you do not have a CLI configuration file, Provision will use the default config_path <comment>{$config_path}</comment>");
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
                throw new \Exception('The config_path and contexts_path folders must exist for Provision to function. Create them manually, or run `provision setup`.');
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
            else {
                throw new RuntimeException('The config_path and contexts_path folders must exist for Provision to function. Create them manually, or run `provision setup`.');
            }
        }

        if ($this->input->isInteractive()) {
            $this->io->pause('Provision CLI configuration check is complete. Press ENTER to continue');
        }
        else {
            $this->io->block('Provision CLI configuration check is complete.');
        }
    }

    /**
     * Step 2: Check and prepare config files and directories.
     * @throws \Exception
     */
    protected function setupServer() {

        $this->io->title('Server Setup');

        $this->io->block('In this section, we will inform provision about services available on your system.');

        $this->io->block('Helpful Tips:');
        $this->io->bulletLite('You can use the <comment>provision save</comment> command to add additional sites and servers to the system.');
        $this->io->bulletLite('You can use the <comment>provision services</comment> command to add services to your servers and sites.');

        $this->io->writeln('');

        $num_servers = count($this->getProvision()->getAllServers());
        if ($num_servers) {
            $this->io->successLite("<comment>$num_servers</comment> Server Context found.");

            if (!$this->input->isInteractive()){
                exit(0);
            }
        }
        else {
            $this->io->warningLite("No servers found.");
            if (!$this->input->isInteractive()){
                exit(0);
            }

            $this->io->pause("You must save at least one server context. Press ENTER to create a server context for your system");

            // Run `provision save` command.
            $command = $this->getApplication()->find('save');
            $parameters = $_SERVER['argv'];
            $parameters['--context_type'] = 'server';
            $parameters['--context'] = 'server_master';
            $input = new ArrayInput($parameters);
            $exit_code = $command->run($input, $this->output);

            if ($exit_code != ResultData::EXITCODE_OK) {
                throw new \Exception('Something went wrong when running `provision save`. Please try again.');
            }
            else {
                $this->io->successLite("Server Context added! You can now add sites or platforms with the `provision save` command. Have fun!");

            }
        }
    }
}

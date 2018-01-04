<?php

namespace Aegir\Provision\Console;

use Aegir\Provision\Provision;
use Drupal\Console\Core\Style\DrupalStyle;
use Robo\Common\IO;
use Symfony\Component\Config\Definition\Exception\Exception;
use Symfony\Component\Console\Exception\InvalidOptionException;
use Symfony\Component\Filesystem\Filesystem;
use Symfony\Component\Filesystem\Exception\IOExceptionInterface;

/**
 * Class Config
 * @package Aegir\Provision\Console
 *
 * Many thanks to pantheon-systems/terminus. Inspired by DefaultConfig
 */
class Config extends ProvisionConfig
{
    const CONFIG_FILENAME = '.provision.yml';

    use IO;

    /**
     * @var Filesystem
     */
    private $fs;

    /**
     * DefaultsConfig constructor.
     */
    public function __construct(DrupalStyle $io = null)
    {
        parent::__construct();
        $this->io = $io;
        $this->fs = new Filesystem();

        $this->set('root', $this->getProvisionRoot());
        $this->set('php', $this->getPhpBinary());
        $this->set('php_version', PHP_VERSION);
        $this->set('php_ini', get_cfg_var('cfg_file_path'));
        $this->set('script', $this->getProvisionScript());

        $os_string = explode(' ', php_uname('v'));
        $os = array_shift($os_string);
        $this->set('os_version', $os);
        $this->set('user_home', $this->getHomeDir());
        
        $this->set('aegir_root', $this->getHomeDir());
        $this->set('script_user', $this->getScriptUser());

        // If user has a ~/.config path, use it.
        if (file_exists($this->getHomeDir() . '/.config')) {
            $this->set('config_path', $this->getHomeDir() . '/.config/provision');
        }
        // Legacy location: /{$HOME}/config
        else {
            $this->set('config_path', $this->getHomeDir() . '/config');
        }

        $this->set('contexts_path', $this->get('config_path') . DIRECTORY_SEPARATOR . Provision::CONTEXTS_PATH);

        $file = $this->get('user_home') . DIRECTORY_SEPARATOR . Config::CONFIG_FILENAME;
        $this->set('console_config_file', $file);
        
        $this->extend(new YamlConfig($this->get('user_home') . DIRECTORY_SEPARATOR . Config::CONFIG_FILENAME));
        $this->extend(new DotEnvConfig(getcwd()));
        $this->extend(new EnvConfig());

        $this->validateConfig();
    }
    
    /**
     * Check configuration values against the current system.
     *
     * @throws \Exception
     */
    protected function validateConfig() {
        // Check that aegir_root is writable.
        if (!is_writable($this->get('aegir_root'))) {
            throw new InvalidOptionException(
                "The folder set to 'aegir_root' ({$this->get('aegir_root')}) is not writable. Fix this or change the aegir_root value in the file {$this->get('console_config_file')}"
            );
        }

        // If config_path does not exist...
        if (!file_exists($this->get('config_path'))) {

            // START: New User!
            // @TODO: Break this out into it's own method or class.
            $this->io->title("Welcome to Provision!");
            $this->io->block([
                "It looks like this is your first time running Provision, because your config_path folder is missing. This is the place Provision stores your metadata and server configuration.",
            ]);

            // Tell the user how to change the config path. Change language if they already have the .provision.yml file.
            if (file_exists($this->get('console_config_path'))) {
                $this->io->commentBlock([
                    'If you would like to change the default Config Path, create a file ' . $this->get('console_config_file') . ' and add:',
                    '    config_path: /path/to/my/provision/config'
                ]);
            }
            else {
                $this->io->commentBlock([
                    'If you would like to change the default Config Path, add the following to the file ' . $this->get('console_config_file'),
                    '    config_path: /path/to/my/provision/config'
                ]);
            }

            // Offer to create the folder for the user.
            if ($this->input()->hasParameterOption(array('--no-interaction', '-n'), false) || $this->io->confirm('Should I create the folders ' . $this->get('config_path') . ' and ' . $this->get('contexts_path') . ' ?')) {
                $this->fs->mkdir($this->get('config_path'), 0700);
                $this->fs->mkdir($this->get('contexts_path'), 0700);

                $this->io->successLite('Created paths successfully.');
                $this->io->writeln('');
            }
        }


        // Check for paths that need to be writable.
        $writable_paths['config_path'] = $this->get('config_path');
        $writable_paths['contexts_path'] = $this->get('contexts_path');
        $errors = [];
        foreach ($writable_paths as $name => $path) {
            if (!is_writable($path)) {
                $errors[] = "The folder set to '$name' ($path) is not writable. Fix this or change the $name value in the file {$this->get('console_config_file')}.";
            }
        }
        if ($errors) {
            throw new InvalidOptionException(implode("\n\n", $errors));
        }

        // Ensure that script_user is the user.
        $real_script_user = $this->getScriptUser();
        if ($this->get('script_user') != $real_script_user) {
            throw new InvalidOptionException(
                "The user set as 'script_user' ({$this->get('script_user')}) is not the currently running user ({$real_script_user}). Change to user {$this->config->get('script_user')} or change the script_user value in the file {{$this->get('console_config_file')}}."
            );
        }
    }
    
    
    /**
     * Get the name of the source for this configuration object.
     *
     * @return string
     */
    public function getSourceName()
    {
        return 'Default';
    }
    
    /**
     * Returns location of PHP with which to run Terminus
     *
     * @return string
     */
    protected function getPhpBinary()
    {
        return defined('PHP_BINARY') ? PHP_BINARY : 'php';
    }
    
    /**
     * Finds and returns the root directory of Provision
     *
     * @param string $current_dir Directory to start searching at
     * @return string
     * @throws \Exception
     */
    protected function getProvisionRoot($current_dir = null)
    {
        if (is_null($current_dir)) {
            $current_dir = dirname(__DIR__);
        }
        if (file_exists($current_dir . DIRECTORY_SEPARATOR . 'composer.json')) {
            return $current_dir;
        }
        $dir = explode(DIRECTORY_SEPARATOR, $current_dir);
        array_pop($dir);
        if (empty($dir)) {
            throw new \Exception('Could not locate root to set PROVISION_ROOT.');
        }
        $dir = implode(DIRECTORY_SEPARATOR, $dir);
        $root_dir = $this->getProvisionRoot($dir);
        return $root_dir;
    }
    
    /**
     * Finds and returns the name of the script running Terminus functions
     *
     * @return string
     */
    protected function getProvisionScript()
    {
        $debug           = debug_backtrace();
        $script_location = array_pop($debug);
        $script_name     = str_replace(
            $this->getProvisionRoot() . DIRECTORY_SEPARATOR,
            '',
            $script_location['file']
        );
        return $script_name;
    }
    
    /**
     * Returns the appropriate home directory.
     *
     * Adapted from Terminus Package Manager by Ed Reel
     * @author Ed Reel <@uberhacker>
     * @url    https://github.com/uberhacker/tpm
     *
     * @return string
     */
    static public function getHomeDir()
    {
        $home = getenv('HOME');
        if (!$home) {
            $system = '';
            if (getenv('MSYSTEM') !== null) {
                $system = strtoupper(substr(getenv('MSYSTEM'), 0, 4));
            }
            if ($system != 'MING') {
                $home = getenv('HOMEPATH');
            }
        }
        return $home;
    }
    
    /**
     * Determine the user running provision.
     */
    static public function getScriptUser() {
        $real_script_user = posix_getpwuid(posix_geteuid());
        return $real_script_user['name'];
    }
}

<?php

namespace Aegir\Provision\Console;

use Aegir\Provision\Common\NotSetupException;
use Aegir\Provision\Common\ProvisionAwareTrait;
use Aegir\Provision\Provision;
use Aegir\Provision\Console\ArgvInput;
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
    const COMPOSER_INSTALL_DEFAULT = 'composer install --no-interaction';

    use IO;
    use ProvisionAwareTrait;

    /**
     * @var Filesystem
     */
    private $fs;

    /**
     * DefaultsConfig constructor.
     */
    public function __construct(ProvisionStyle $io = null, $validate = TRUE)
    {
        parent::__construct();
        $this->io = $io;
        $this->fs = new Filesystem();

        $this->set('root', $this->getProvisionRoot());
        $this->set('php', $this->getPhpBinary());
        $this->set('php_version', PHP_VERSION);
        $this->set('php_ini', get_cfg_var('cfg_file_path'));
        $this->set('script', $this->getProvisionScript());
        $this->set('interactive_task_sleep', 500);

        $os_string = explode(' ', php_uname('v'));
        $os = array_shift($os_string);
        $this->set('os_version', $os);
        $this->set('user_home', $this->getHomeDir());
        
        $this->set('aegir_root', $this->getHomeDir());
        $this->set('script_user', $this->getScriptUser());
        $this->set('script_uid', $this->getScriptUid());
        $this->set('web_user', $this->getWebUser());
        $this->set('web_user_uid', $this->getWebUserUid());


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

        try {
            $this->extend(new YamlConfig($this->get('user_home') . DIRECTORY_SEPARATOR . Config::CONFIG_FILENAME));
        }
        catch (\TypeError $e) {
            throw new \Exception('Unable to parse YAML file from ' . $this->get('user_home') . DIRECTORY_SEPARATOR . Config::CONFIG_FILENAME);
        }

        $this->extend(new DotEnvConfig(getcwd()));
        $this->extend(new EnvConfig());

        if ($validate) {
            $this->validateConfig();
        }
    }

    /**
     * Override io() to return ProvisionStyle instead of SymfonyStyle.
     *
     * @return ProvisionStyle
     */
    protected function io()
    {
        if (!$this->io) {
            $this->io = new ProvisionStyle($this->input(), $this->output());
        }
        return $this->io;
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

        // Check for missing everything. Tell the user to run the setup command.
        // @TODO: Run the setup command here instead. I poked and prodded but could not get it to work. Config is instantiated before Application
//        if (
//            !file_exists($this->get('config_path')) &&
//            !file_exists($this->get('console_config_file'))
//        ) {
//            throw new NotSetupException();
//        }


        // Check for paths that need to be writable.
        $writable_paths['config_path'] = $this->get('config_path');
        $writable_paths['contexts_path'] = $this->get('contexts_path');
        $errors = [];
        foreach ($writable_paths as $name => $path) {
            if (!file_exists($path)) {

                $errors[] = "The '$name' folder ($path) does not exist. You must create it or change the value for '$name' in the file {$this->get('console_config_file')}.";

            }
            elseif (!is_writable($path)) {
                $errors[] = "The '$name' folder ($path) is not writable. Fix this or change the value for '$name' in the file {$this->get('console_config_file')}.";
            }
        }
        if ($errors) {
            throw new Exception(implode("\n\n", $errors));
        }

        // Ensure that script_user is the user.
        if ($this->get('script_user') != $this->getScriptUser()) {
            throw new InvalidOptionException(
                "The user set as 'script_user' ({$this->get('script_user')}) is not the currently running user ({$this->getScriptUser()}). Switch user to user {$this->get('script_user')} or change the script_user value in the file {{$this->get('console_config_file')}}."
            );
        }

        // @TODO: Ensure that web_user exists. Right now all that matters is web_user_uid

        // Ensure that script user is a member of web user group.
        if (!$this->isUserInWebGroup($this->get('web_user_uid'))) {

            $this->io()->warningLite("Your user is not in the web group.");
            $this->io()->helpBlock([
                "To add your user to the web user group, run one of the following commands:",
                "",
                "    mac: sudo dseditgroup -o edit -a {$this->get('script_user')} -t user {$this->get('web_user')}",
                "    linux: sudo usermod -aG {$this->get('web_user')} {$this->get('script_user')}",
            ]);

          throw new InvalidOptionException(
            "The current user ({$this->get('script_user')}) is not in the group '{$this->get('web_user')}' [{$this->get('web_user_uid')}]. Please add your user '{$this->config->get('script_user')}' to the group '{$this->get('web_user')}' or change the web_user or web_user_uid values in the file {{$this->get('console_config_file')}}.}"
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

    /**
     * Determine the user running provision.
     */
    static public function getScriptUid() {
        return posix_getuid();
    }

    /**
     * Return the UID of the discovered web user group on the current system.
     */
    static public function getWebUser() {
        return Provision::defaultWebGroup();
    }

    /**
     * Return the UID of the discovered web user group on the current system.
     */
    static public function getWebUserUid() {
        $info = posix_getpwnam(Provision::defaultWebGroup());
        return $info['uid'];
    }

    /**
     * @return bool
     */
    public function isUserInWebGroup($web_user_uid) {
        $gids = posix_getgroups();
        return in_array($web_user_uid, $gids);
    }
}

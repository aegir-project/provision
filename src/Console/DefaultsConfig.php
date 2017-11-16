<?php

namespace Aegir\Provision\Console;

/**
 * Class DefaultsConfig
 * @package Aegir\Provision\Console
 *
 * Many thanks to pantheon-systems/terminus.
 */
class DefaultsConfig extends ProvisionConfig
{
    /**
     * DefaultsConfig constructor.
     */
    public function __construct()
    {
        parent::__construct();
        
        $this->set('root', $this->getProvisionRoot());
        $this->set('php', $this->getPhpBinary());
        $this->set('php_version', PHP_VERSION);
        $this->set('php_ini', get_cfg_var('cfg_file_path'));
        $this->set('script', $this->getProvisionScript());
        $this->set('os_version', php_uname('v'));
        $this->set('user_home', $this->getHomeDir());
        $this->set('aegir_root', $this->getHomeDir());
        $this->set('script_user', $this->getScriptUser());
        $this->set('config_path', $this->getHomeDir() . '/config');
    
        $this->validateConfig();
    }
    
    /**
     * Check configuration values against the current system.
     *
     * @throws \Exception
     */
    protected function validateConfig() {
        // Check that aegir_root is writable.
        // @TODO: Create some kind of Setup functionality.
        if (!is_writable($this->config->get('aegir_root'))) {
            throw new \Exception(
                "There is an error with your configuration. The folder set to 'aegir_root' ({$this->config['aegir_root']}) is not writable. Fix this or change the aegir_root value in the file {$this->config_path}."
            );
        }
        // Check that config_path exists and is writable.
        if (!file_exists($this->config->get('config_path'))) {
            throw new \Exception(
                "There is an error with your configuration. The folder set to 'config_path' ({$this->config['config_path']}) does not exist. Create it or change the config_path value in the file {$this->config_path}."
            );
        }
        elseif (!is_writable($this->config->get('config_path'))) {
            throw new \Exception(
                "There is an error with your configuration. The folder set to 'config_path' ({$this->config['config_path']}) is not writable. Fix this or change the config_path value in the file {$this->config_path}."
            );
        }
        elseif (!file_exists($this->config->get('config_path') . '/provision')) {
            mkdir($this->config->get('config_path') . '/provision');
        }
        
        // Ensure that script_user is the user.
        $real_script_user = $this->getScriptUser();
        if ($this->config->get('script_user') != $real_script_user) {
            throw new \Exception(
                "There is an error with your configuration. The user set as 'script_user' ({$this->config['script_user']}) is not the currently running user ({$real_script_user}). Change to user {$this->config->get('script_user')} or change the script_user value in the file {$this->config_path}."
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
    protected function getHomeDir()
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
    protected function getScriptUser() {
        $real_script_user = posix_getpwuid(posix_geteuid());
        return $real_script_user['name'];
    }
}

<?php

namespace Aegir\Provision\Console;

use Symfony\Component\Config\Definition\Builder\TreeBuilder;
use Symfony\Component\Config\Definition\ConfigurationInterface;
use Symfony\Component\Config\Definition\Processor;
use Symfony\Component\Yaml\Dumper;
use Symfony\Component\Filesystem\Filesystem;
use Symfony\Component\Filesystem\Exception\IOExceptionInterface;
use Symfony\Component\Yaml\Yaml;


/**
 * Class Config.
 */
class Config implements ConfigurationInterface
{

    /**
     * Configuration values array.
     *
     * @var array
     */
    private $config = [];

    /**
     * Path to config YML file.
     *
     * @var string
     */
    private $config_path = '';

    /**
     * Filename of config YML file.
     *
     * @var string
     */
    private $config_filename = '.provision.yml';

    /**
     * {@inheritdoc}
     */
    public function __construct()
    {
        $this->config_path = $this->getHomeDir().'/'.$this->config_filename;

        try {
            $processor = new Processor();
            $configs = func_get_args();
            if (file_exists($this->config_path)) {
                $configs[] = Yaml::parse(file_get_contents($this->config_path));
            }
            $this->config = $processor->processConfiguration($this, $configs);
        } catch (\Exception $e) {
            throw new \Exception(
              'There is an error with your configuration: '.$e->getMessage()
            );
        }


    }

    /**
     * {@inheritdoc}
     */
    public function getConfigTreeBuilder()
    {
        $tree_builder = new TreeBuilder();
        $root_node = $tree_builder->root('aegir');
        $root_node
          ->children()
          ->scalarNode('aegir_root')
          ->defaultValue('/var/aegir')
          ->end()
          ->scalarNode('script_user')
          ->defaultValue('aegir')
          ->end()
          ->scalarNode('config_path')
          ->defaultValue('/var/aegir/config')
          ->end()
          ->end();;

        return $tree_builder;
    }

    /**
     * Get a config param value.
     *
     * @param string $key
     *                    Key of the param to get.
     *
     * @return mixed|null
     *                    Value of the config param, or NULL if not present.
     */
    public function get($key, $name = null)
    {
        if ($name) {
            return array_key_exists(
              $name,
              $this->config[$key]
            ) ? $this->config[$key][$name] : null;
        } else {
            return $this->has($key) ? $this->config[$key] : null;
        }
    }

    /**
     * Check if config param is present.
     *
     * @param string $key
     *                    Key of the param to check.
     *
     * @return bool
     *              TRUE if key exists.
     */
    public function has($key)
    {
        return array_key_exists($key, $this->config);
    }

    /**
     * Set a config param value.
     *
     * @param string $key
     *                    Key of the param to get.
     * @param mixed $val
     *                    Value of the param to set.
     *
     * @return bool
     */
    public function set($key, $val)
    {
        return $this->config[$key] = $val;
    }

    /**
     * Get all config values.
     *
     * @return array
     *               All config galues.
     */
    public function all()
    {
        return $this->config;
    }

    /**
     * Add a config param value to a config array.
     *
     * @param string $key
     *                            Key of the group to set to.
     * @param string|array $names
     *                            Name of the new object to set.
     * @param mixed $val
     *                            Value of the new object to set.
     *
     * @return bool
     */
    public function add($key, $names, $val)
    {
        if (is_array($names)) {
            $array_piece = &$this->config[$key];
            foreach ($names as $name_key) {
                $array_piece = &$array_piece[$name_key];
            }

            return $array_piece = $val;
        } else {
            return $this->config[$key][$names] = $val;
        }
    }

    /**
     * Remove a config param from a config array.
     *
     * @param $key
     * @param $name
     *
     * @return bool
     */
    public function remove($key, $name)
    {
        if (isset($this->config[$key][$name])) {
            unset($this->config[$key][$name]);

            return true;
        } else {
            return false;
        }
    }

    /**
     * Saves the config class to file.
     *
     * @return bool
     */
    public function save()
    {

        // Create config folder if it does not exist.
        $fs = new Filesystem();
        $dumper = new Dumper();

        try {
            $fs->dumpFile($this->config_path, $dumper->dump($this->config, 10));

            return true;
        } catch (IOExceptionInterface $e) {
            return false;
        }
    }

    /**
     * Returns the appropriate home directory.
     *
     * Adapted from Terminus Package Manager by Ed Reel
     *
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
}

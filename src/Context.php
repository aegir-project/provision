<?php
/**
 * @file
 * Provides the Aegir\Provision\Context class.
 */

namespace Aegir\Provision;

use Symfony\Component\Config\Definition\Processor;
use Symfony\Component\Yaml\Yaml;

/**
 * Base context class.
 */
class Context
{

    /**
     * @var string
     * Name for saving aliases and referencing.
     */
    public $name = null;

    /**
     * @var string
     * 'server', 'platform', or 'site'.
     */
    public $type = null;

    /**
     * @var array
     * Properties that will be persisted by provision-save. Access as object
     * members, $evironment->property_name. __get() and __set handle this. In
     * init(), set defaults with setProperty().
     */
    protected $properties = [];

    /**
     * Constructor for the context.
     */
    function __construct($name, $options, $console_config)
    {
        $this->name = $name;
        
        $this->console_config = $console_config;
        $this->config_path = $console_config['config_path'] . '/provision/' . $this->type . '.' . $this->name . '.yml';
        
        $configs = [];
        
        try {
            $processor = new Processor();
            if (file_exists($this->config_path)) {
                $configs[] = Yaml::parse(file_get_contents($this->config_path));
            }
            else {
                // Load command line options into properties
                foreach ($this->option_documentation() as $option => $description) {
                    $this->properties[$option] = $options[$option];
                }
            }
            
            $this->config = $processor->processConfiguration($this, $configs);
        } catch (\Exception $e) {
            throw new \Exception(
                "There is an error with the configuration for $this->type $this->name: " . $e->getMessage()
            );
        }
    }
}

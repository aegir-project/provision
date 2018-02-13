<?php

namespace Aegir\Provision\Console;

use Symfony\Component\Yaml\Yaml;

/**
 * Class YamlConfig
 * @package Aegir\Provision\Console
 *
 * Many thanks to pantheon-systems/terminus.
 */
class YamlConfig extends ProvisionConfig
{
    /**
     * YamlConfig constructor.
     * @param string $yml_path The path to the yaml file.
     */
    public function __construct($yml_path)
    {
        parent::__construct();
        
        $this->setSourceName($yml_path);
        $file_config = file_exists($yml_path) ? Yaml::parse(file_get_contents($yml_path)) : [];
        if (!is_null($file_config)) {
            $this->fromArray($file_config);
        }
    }
}

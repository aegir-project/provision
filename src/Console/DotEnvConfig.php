<?php

namespace Aegir\Provision\Console;

use Aegir\Provision\Provision;

/**
 * Class DotEnvConfig
 * @package Pantheon\Terminus\Config
 */
class DotEnvConfig extends ProvisionConfig
{
    /**
     * @var string
     */
    protected $file;
    
    /**
     * DotEnvConfig constructor.
     */
    public function __construct($dir)
    {
        parent::__construct();
        
        $file = $dir . '/.env';
        $this->setSourceName($file);
        
        // Load environment variables from __DIR__/.env
        if (file_exists($file)) {
            // Remove comments (which start with '#')
            $lines = file($file);
            $lines = array_filter($lines, function ($line) {
                return strpos(trim($line), '#') !== 0;
            });
            $info = parse_ini_string(implode($lines, "\n"));
            $this->fromArray($info);
        }
    }
}

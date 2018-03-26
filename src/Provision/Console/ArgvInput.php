<?php

namespace Aegir\Provision\Console;

use \Symfony\Component\Console\Input\ArgvInput as ArgvInputBase;
use \Symfony\Component\Console\Input\InputDefinition;

class ArgvInput extends ArgvInputBase {

    /**
     * @var string The name of the active context, extracted from the first argument if it has "@" prefix.
     */
    public $activeContextName = NULL;

    /**
    * @param array|null           $argv       An array of parameters from the CLI (in the argv format)
    * @param InputDefinition|null $definition A InputDefinition instance
    */
    public function __construct(array $argv = null, InputDefinition $definition = null)
    {
        // If @alias is used, swap it out with --context=
        if (isset($argv[1]) && strpos($argv[1], '@') === 0) {
            $context_name = ltrim($argv[1], '@');
            $argv[1] = "--context={$context_name}";
            $this->activeContextName = $context_name;
        }
        // If --context option is used, use that.
        elseif ($argv_filtered = array_filter($argv, function ($key) {
            return strpos($key, '--context=') === 0;
        })) {
            $context_option = array_pop($argv_filtered);
            $context_name = substr($context_option, strlen('--context='));
            $this->activeContextName = $context_name;
        }
        parent::__construct($argv, $definition);
    }
}
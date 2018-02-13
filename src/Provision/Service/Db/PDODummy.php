<?php

namespace Aegir\Provision\Service\Db;

/**
 * Class PDODummy
 * @package Aegir\Provision\Service\Db
 *
 * This class basically exists so that DbMySqlService.php can still call $grants->fetch().
 *
 * If any other plugins wanted to use the Docker query(), they have to load data from the lines output.
 *
 */
class PDODummy {

    function __construct($lines)
    {
        $this->lines = explode(PHP_EOL, $lines);
    }

    /**
     * @return array
     */
    function fetch() {
        $item = explode("\t", current($this->lines));
        next($this->lines);
        return $item;
    }
}
<?php

namespace Aegir\Provision;

class Task {
    
    function execute($callable)
    {
        $this->callable = $callable;
        return $this;
    }
    function success($message) {
       
        $this->success = $message;
        return $this;
    }
    function failure($message) {
        $this->failure = $message;
        return $this;
    }
    
    static function new() {
        return new Task();
    }
    
}
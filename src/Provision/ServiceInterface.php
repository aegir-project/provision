<?php

namespace Aegir\Provision;

interface ServiceInterface
{

    /**
     * Triggered on `provision verify` command on Site contexts.
     */
    public function verifySite();


    /**
     * Triggered on `provision verify` command on Platform contexts.
     */
    public function verifyPlatform();


    /**
     * Triggered on `provision verify` command on Site contexts.
     */
    public function verifyServer();

}
<?php

namespace Aegir\Provision\Service;

interface DockerServiceInterface
{

    /**
     * The docker image to use for this service.
     *
     * @return string
     */
    public function dockerImage();

    /**
     * Return array of data for this docker compose service.
     *
     * @return array
     */
    public function dockerComposeService();

}
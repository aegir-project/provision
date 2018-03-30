<?php

namespace Aegir\Provision\Common;

use Aegir\Provision\Context;

trait DockerContainerTrait
{
    /**
    * @var string The name of this service's container.
    */
    private $containerName;


    /**
    * @var string The tag for this services's container.
    */
    private $containerTag;

    /**
     * @return \Robo\Result;
     */
    private function containerExists() {
        $provision = $this->getProvision();

        return $provision->getTasks()
            ->taskExec("docker ps -a -q -f name={$this->containerName}")
            ->storeState('container_id')
            ->taskFileSystemStack()
            ->defer(
                function ($task, $state) use ($provision) {
                    $container = $state['container_id'];
                    if ($container) {

                        //Check that it is running
                        $provision->getTasks()
                            ->taskExec("docker inspect -f '{{.State.Running}}' {$this->containerName}")
                            ->silent(!$provision->getOutput()->isVerbose())
                            ->printOutput(false)
                            ->storeState('running')
                            ->taskFileSystemStack()
                            ->defer(
                                function ($task, $state) use ($provision) {

                                    if ($state['running'] == 'true') {
//                                          $provision->io()->successLite('Container is already running: ' . $this->containerName);
                                        // @TODO: Figure out how to change the "success" message of a task.
//                                          $this->success('Container is already running: ' . $this->containerName);
                                    }
                                    // If not running, try to start it.
                                    else {
                                        $startResult = $provision->getTasks()
                                            ->taskExec("docker start {$this->containerName}")
                                            ->silent(!$provision->getOutput()->isVerbose())
                                            ->run()
                                        ;

                                        if ($startResult->wasSuccessful()) {
//                                              $provision->io()->successLite('Existing container found. Restarted container ' . $this->containerName);
                                        }
                                        else {
//                                              $provision->io()->errorLite('Unable to restart docker container: ' . $this->containerName);
                                            throw new \Exception('Unable to restart docker container: ' . $this->containerName);
                                        }
                                    }
                                })
                            ->run();

                    }

                    # Docker container not found. Start it.
                    else {
                        $configVolumeHost = $provision->getConfig()->get('config_path') . DIRECTORY_SEPARATOR . $this->provider->name;
                        $configVolumeGuest = $this->provider->getProperty('aegir_root') . '/config/' . $this->provider->name;

                        $container = $provision->getTasks()->taskDockerRun($this->containerTag)
                            ->detached()
                            ->publish($this->getProperty($this::SERVICE . '_port'), $this::SERVICE_DEFAULT_PORT)
                            ->name($this->containerName)
                            ->volume($configVolumeHost, $configVolumeGuest)
                            ->silent(!$provision->getOutput()->isVerbose())
//                            ->option('rm')
                            ->interactive();

                        // Lookup all subscribers (all platforms that use this web service) and map volumes for root.
                        foreach ($this->getAllSubscribers() as $platform) {
                            if (!empty($platform->getProperty('root'))) {
                                $container->volume($platform->getProperty('root'), $this->mapContainerPath($platform->getProperty('root')));
                            }
                        }

                        $result = $container->run();

                        if (!$result->wasSuccessful()) {
                            throw new \Exception('Unable to run docker container: ' . $this->containerName);
                        }
                    }
                }
            )
            ->run();
    }
}

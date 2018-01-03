<?php

namespace Aegir\Provision\Service\Db;

use Aegir\Provision\Common\DockerContainerTrait;
use Aegir\Provision\Context;
use Aegir\Provision\Provision;
use Symfony\Component\Console\Exception\RuntimeException;

/**
 * Class HttpMySqlDockerService
 *
 * @package Aegir\Provision\Service\Db
 */
class DbMysqlDockerService extends DbMysqlService
{
    const SERVICE_TYPE = 'mysqlDocker';
    const SERVICE_TYPE_NAME = 'MySQL on Docker';

    use DockerContainerTrait;

    /**
     * DbMysqlDockerService constructor.
     * @param $service_config
     * @param Context $provider_context
     */
    function __construct($service_config, Context $provider_context)
    {
        parent::__construct($service_config, $provider_context);

        $this->containerName = "provision_db_{$this->provider->name}";
        $this->containerTag = "mariadb";

    }

    static function server_options () {
        $options = parent::server_options();
        $options['db_port'] = 3306;
        return $options;
    }


    public function verifyServer()
    {
        $tasks = [];

        // Docker run
        $provision = $this->getProvision();
        $tasks['db.docker.run'] = $this->getProvision()->newTask()
            ->start('Running MariaDB docker image...')
            ->execute(function () use ($provision) {

                // Check for existing container.
                $provision = $this->getProvision();
                $containerExists = $provision->getTasks()
                    ->taskExec("docker ps -a -q -f name={$this->containerName}")
                    ->silent(!$this->getProvision()->getOutput()->isVerbose())
                    ->printOutput(false)

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
                                $container = $provision->getTasks()->taskDockerRun($this->containerTag)
                                    ->detached()
                                    ->publish($this->getProperty($this::SERVICE . '_port'), $this::SERVICE_DEFAULT_PORT)
                                    ->name($this->containerName)
                                    ->silent(!$provision->getOutput()->isVerbose())
//                            ->option('rm')
                                        ->env('MYSQL_ROOT_PASSWORD', $this->creds['pass'])
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
                if (!$containerExists->wasSuccessful()) {
                    throw new RuntimeException('Unable to check for the container: ' . $containerExists->getMessage(), 1);
                }
            });
        $tasks = array_merge($tasks, parent::verifyServer());
        return $tasks;
    }


}
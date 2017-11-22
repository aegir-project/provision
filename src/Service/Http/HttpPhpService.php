<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Robo\ProvisionCollectionBuilder;
use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfiguration;
use Aegir\Provision\Service\Http\Apache\Configuration\ServerConfiguration;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfiguration;
use Aegir\Provision\Service\HttpService;
use Aegir\Provision\ServiceSubscription;
use Robo\Result;
use Aegir\Provision\Service\Http\Php\PhpServer;
use Symfony\Component\Process\Process;

/**
 * Class HttpPhpService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpPhpService extends HttpService
{
    const SERVICE_TYPE = 'php';
    const SERVICE_TYPE_NAME = 'PHP Server';


    public function verifyServer()
    {

        $host = $this->getContext()->getProperty('remote_host');
        $port = $this->getProperty('http_port');

        $this->getProvision()->getLogger()->info('Running server at {host}:{port}', [
            'host' => $host,
            'port' => $port,
        ]);
//

        $this->getContext()->getBuilder()->build(PhpServer::class, ['port' => $port]);

//        $server = new PhpServer($port);

        /** @var PhpServer $server */
        $server = $this->getContext()->getBuilder()->task(PhpServer::class, ['port' => $port]);

        $server->host($host);
        $server->dir(__DIR__ . '/Php/servertest');

        $server->background();
        $server->run();
//
        $pid = $server->getProcess()->getPid();

        $this->getProvision()->getLogger()->info('Server started at {host}:{port} running as process {pid}', [
            'host' => $host,
            'port' => $port,
            'pid' => $pid,
        ]);


    }

    public function verifySite()
    {
    }

    public function verifySubscription(ServiceSubscription $subscription)
    {

        print_r($subscription->context->getProperty('root'));
        $this->application->getBuilder()->taskServer($this->getProperty('http_port'))
            ->dir('.');;


    }
}

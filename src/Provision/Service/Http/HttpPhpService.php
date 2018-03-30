<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Robo\ProvisionCollectionBuilder;
use Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfigFile;
use Aegir\Provision\Service\Http\Apache\Configuration\ServerConfigFile;
use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfigFile;
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

        $this->host = $this->getContext()->getProperty('remote_host');
        $this->port = $this->getProperty('http_port');

        $this->getProvision()->getLogger()->info('Running server at {host}:{port}', [
            'host' => $this->host,
            'port' => $this->port,
        ]);

        $tasks['http.php.launch'] = $this->getProvision()->newTask()
            ->success('Internal PHP Server has been verified.')
            ->failure('Something went wrong when launching the server.')
            ->execute(function () {
                
                $process = new Process(sprintf("php -S %s:%d -t %s &",$this->host, $this->port, __DIR__ . '/Php/servertest'));
                $process->start();
                
                $pid = $process->getPid();
                
                $this->getProvision()->io()->successLite("PHP server running at http://{$this->host}:{$this->port} on PID {$pid}");
            });
        
        return $tasks;
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

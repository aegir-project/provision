<?php
/**
 * @file
 * The base Provision HttpService class.
 *
 * @see \Provision_Service_http
 */

namespace Aegir\Provision\Service;

//require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';

use Aegir\Provision\Service;
use Aegir\Provision\ServiceInterface;
use Aegir\Provision\ServiceSubscription;
use Aegir\Provision\Task;
use Consolidation\AnnotatedCommand\CommandFileDiscovery;

/**
 * Class HttpService
 *
 * @package Aegir\Provision\Service
 */
class HttpService extends Service implements ServiceInterface {
  const SERVICE = 'http';
  const SERVICE_NAME = 'Web Server';

  protected $ssl_enabled = FALSE;


    /**
     * Implements Service::server_options()
     *
     * @return array
     */
    static function server_options()
    {
        return [
            'http_port' => 'The port which the web service is running on.',
            'web_group' => 'server with http: OS group for permissions; working default will be attempted',
            'web_disable_url' => 'server with http: URL disabled sites are redirected to; default {master_url}/hosting/disabled',
            'web_maintenance_url' => 'server with http: URL maintenance sites are redirected to; default {master_url}/hosting/maintenance',
            'restart_command' => 'The command to reload the web server configuration;'
        ];
    }
    
    /**
     * List context types that are allowed to subscribe to this service.
     * @return array
     */
    static function allowedContexts() {
        return [
            'platform'
        ];
    }

    /**
     * React to `provision verify` command when run on a subscriber, to verify the service's provider.
     *
     * This is used to allow skipping of the service restart.
     */
    function verifyServer()
    {
        return [
            'http.configuration' => $this->getProvision()->newTask()
                ->execute(function() {
                    $this->writeConfigurations();
                })
                ->success('Wrote web server configuration files.')
                ->failure('Unable to write config files for this service.'),
            
            'http.restart' => $this->getProvision()->newTask()
                ->execute(function() {
                    $this->restartService();
                })
                ->success('Web service restarted.')
                ->failure('Never shown: exception message used instead.'),
        ];
    }

    /**
     * React to the `provision verify` command on Server contexts
     */
    function verifySite() {
        $this->subscription = $this->getContext()->getSubscription('http');

        $tasks = [];
        $tasks['http.site.configuration'] =  $this->getProvision()->newTask()
            ->success('Wrote site configuration files.')
            ->failure('Unable to write site configuration files.')
            ->execute(function () {
                $this->writeConfigurations($this->subscription);
            })
        ;
        $tasks['http.site.service'] =  $this->getProvision()->newTask()
            ->success('Restarted web server.')
            ->failure('Unable to restart web service.')
            ->execute(function () {
                $this->restartService();
            })
        ;
        return $tasks;
    }

    function verifyPlatform() {
        $tasks = [];
        $tasks['http.platform.configuration'] =  $this->getProvision()->newTask()
                ->success('Wrote platform configuration to ...')
                ->failure('Unable to write platform configuration file.')
                ->execute(function () {
                    $this->writeConfigurations($this->getContext()->getSubscription('http'));

                })
        ;
        $tasks = array_merge($tasks, $this->verifyServer());
        return $tasks;
    }

    //
//    /**
//   * Support the ability to cloak the database credentials using environment variables.
//   */
//  function cloaked_db_creds() {
//    return FALSE;
//  }
//
//
//  function verify_server_cmd() {
//    $this->create_config($this->context->type);
//    $this->parse_configs();
//  }
//
//  function verify_platform_cmd() {
//    $this->create_config($this->context->type);
//    $this->parse_configs();
//  }
//
//  function verify_site_cmd() {
//    $this->create_config($this->context->type);
//    $this->parse_configs();
//  }
//
//
//  /**
//   * Register the http handler for platforms, based on the web_server option.
//   */
//  static function subscribe_platform($context) {
//    $context->setProperty('web_server', '@server_master');
//    $context->is_oid('web_server');
//    $context->service_subscribe('http', $context->web_server->name);
//  }

}

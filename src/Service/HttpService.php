<?php
/**
 * @file
 * The base Provision HttpService class.
 *
 * @see \Provision_Service_http
 */

namespace Aegir\Provision\Service;

//require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';

use Aegir\Provision\Provision;
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
  const SERVICE_DEFAULT_PORT = 80;

  protected $ssl_enabled = FALSE;


    /**
     * Implements Service::server_options()
     *
     * @return array
     */
    static function server_options()
    {
        return [
            // @TODO: Add ->validate() that checks if port 80 is available.
            // See DrupalConsole "server" command.
            'http_port' => Provision::newProperty()
                ->description('The port which the web service is running on.')
                ->defaultValue(80)
                ->required()
            ,

            // @TODO: Add->validate() that checks if the web_group exists.
            'web_group' => Provision::newProperty()
                ->description('Web server group.')
                ->defaultValue(Provision::defaultWebGroup())
                ->required()
            ,

            // @TODO: Add->validate that tries to run this command.
            'restart_command' => Provision::newProperty()
                ->description('The command to reload the web server configuration.')
                ->defaultValue(function () {
                    return self::default_restart_cmd();
                })
                ->required()
            ,

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
                ->start('Writing web server configuration...')
                ->execute(function() {
                    return $this->writeConfigurations()? 0: 1;
                })
            ,
            'http.restart' => $this->getProvision()->newTask()
                ->start('Restarting web server...')
                ->execute(function() {
                    return $this->restartService()? 0: 1;
                })
        ];
    }

    /**
     * React to the `provision verify` command on Server contexts
     */
    function verifySite() {
        $this->subscription = $this->getContext()->getSubscription('http');

        $tasks = [];
        $tasks['http.site.configuration'] =  $this->getProvision()->newTask()
            ->start('Writing site web server configuration...')
            ->execute(function () {
                return $this->writeConfigurations($this->getContext())? 0: 1;
            })
        ;
        $tasks['http.site.service'] =  $this->getProvision()->newTask()
            ->start('Restarting web server...')
            ->execute(function () {
                return $this->restartService()? 0: 1;
            })
        ;
        return $tasks;
    }

    function verifyPlatform() {
        $tasks = [];
        $tasks['http.platform.configuration'] =  $this->getProvision()->newTask()
                ->start('Writing platform web server configuration...')
                ->execute(function () {
                    $this->writeConfigurations($this->getContext())? 0: 1;
                })
        ;
        $tasks = array_merge($tasks, $this->verifyServer());
        return $tasks;
    }

    /**
     * Return the default restart command for this service.
     */
    public static function default_restart_cmd() {
        return '';
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

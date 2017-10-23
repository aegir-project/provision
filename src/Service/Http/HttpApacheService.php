<?php
/**
 * @file
 * The Provision HttpApacheService class.
 *
 * @see \Provision_Service_http_apache
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Service\HttpService;

/**
 * Class HttpApacheService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpApacheService extends HttpService
{
  const SERVICE_TYPE = 'apache';
  const SERVICE_TYPE_NAME = 'Apache';
  
  /**
   * Returns array of Configuration classes for this service.
   *
   * @see Provision_Service_http_apache::init_server();
   *
   * @return array
   */
  public function getConfigurations()
  {
    $configs['server'][] = '\Aegir\Provision\Service\Http\Apache\Configuration\ServerConfiguration';
    $configs['platform'][] = '\Aegir\Provision\Service\Http\Apache\Configuration\PlatformConfiguration';
    $configs['site'][] = '\Aegir\Provision\Service\Http\Apache\Configuration\SiteConfiguration';
    return $configs;
  }
  
  /**
   * Respond to the `provision verify` command.
   */
  public function verify() {
//      print "VERIFY APACHE SERVER!";
      parent::verify();
  }
}

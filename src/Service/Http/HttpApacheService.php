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
}

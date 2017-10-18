<?php
/**
 * @file
 * The Provision HttpNginxService class.
 *
 * @see \Provision_Service_http_Nginx
 */

namespace Aegir\Provision\Service\Http;

use Aegir\Provision\Service\HttpService;

/**
 * Class HttpNginxService
 *
 * @package Aegir\Provision\Service\Http
 */
class HttpNginxService extends HttpService
{
  const SERVICE_TYPE = 'nginx';
  const SERVICE_TYPE_NAME = 'NGINX';
}

<?php
/**
 * @file
 * The Provision HttpNginxService class.
 *
 * @see \Provision_Service_http_Nginx
 */

namespace Aegir\Provision\Service\Db;

use Aegir\Provision\Service\DbService;

/**
 * Class DbMysqlService
 *
 * @package Aegir\Provision\Service\Db
 */
class DbMysqlService extends DbService
{
  const SERVICE_TYPE = 'mysql';
  const SERVICE_TYPE_NAME = 'MySQL';
}

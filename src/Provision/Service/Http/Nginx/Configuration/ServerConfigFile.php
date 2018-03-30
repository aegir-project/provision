<?php
/**
 * @file Server.php
 *
 *       Apache Configuration for Server Context.
 *
 *       This class represents the file at /var/aegir/config/apache.conf.
 *
 *
 * @see \Provision_Config_Apache_Server
 * @see \Provision_Config_Http_Server
 * @see \Provision_Config_Http_Server
 */

namespace Aegir\Provision\Service\Http\Nginx\Configuration;

use Aegir\Provision\ConfigFile;
use Aegir\Provision\Provision;
use Aegir\Provision\Service\Http\Apache\Configuration\ServerConfigFile as BaseServerConfigFile;

class ServerConfigFile extends BaseServerConfigFile {
  
  const SERVICE_TYPE = 'nginx';

}
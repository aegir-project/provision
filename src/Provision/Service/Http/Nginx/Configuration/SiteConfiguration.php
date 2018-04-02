<?php
/**
 * @file SiteConfiguration.php
 *
 * NGINX Configuration for Site Context.
 */

namespace Aegir\Provision\Service\Http\Nginx\Configuration;

use Aegir\Provision\Service\Http\Apache\Configuration\SiteConfigFile as BaseSiteConfigFile;

class SiteConfiguration extends BaseSiteConfigFile {

    const SERVICE_TYPE = 'nginx';

    public $template = 'templates/vhost.tpl.php';

}
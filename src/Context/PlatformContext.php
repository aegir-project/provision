<?php

namespace Aegir\Provision\Context;

use Aegir\Provision\Context;

/**
 * Class PlatformContext
 *
 * @package Aegir\Provision\Context
 *
 * @see \Provision_Context_platform
 */
class PlatformContext extends Context {
  static function option_documentation() {
    $options = array(
      'root' => 'platform: path to a Drupal installation',
      'server' => 'platform: drush backend server; default @server_master',
      'web_server' => 'platform: web server hosting the platform; default @server_master',
      'makefile' => 'platform: drush makefile to use for building the platform if it doesn\'t already exist',
      'make_working_copy' => 'platform: Specifiy TRUE to build the platform with the Drush make --working-copy option.',
    );
    return $options;
  }
}

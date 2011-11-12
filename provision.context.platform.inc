<?php
// $Id$

/**
 * @file Provision named context platform class.
 */


/**
 * Class for the platform context.
 */
class provisionContext_platform extends provisionContext {
  public $parent_key = 'server';

  static function option_documentation() {
    return array(
      '--root' => 'platform: path to a Drupal installation',
      '--server' => 'platform: drush backend server; default @server_master',
      '--web_server' => 'platform: web server hosting the platform; default @server_master',
      '--makefile' => 'platform: makefile to use in building the platform',
    );
  }

  function init_platform() {
    $this->setProperty('root');
    $this->setProperty('makefile', '');
  }
}

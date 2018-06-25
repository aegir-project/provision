<?php

/**
 * @file Provision named context platform class.
 */


/**
 * Class for the platform context.
 */
class Provision_Context_platform extends Provision_Context {
  public $parent_key = 'server';

  static function option_documentation() {
    return array(
      'publish_path' => 'platform: path to a Drupal installation',
      'root' => 'platform: path to a Drupal installation',
      'server' => 'platform: drush backend server; default @server_master',
      'web_server' => 'platform: web server hosting the platform; default @server_master',
      'makefile' => 'platform: drush makefile to use for building the platform if it doesn\'t already exist',
      'make_working_copy' => 'platform: Specifiy TRUE to build the platform with the Drush make --working-copy option.',
    );
  }

  function init_platform() {
    $this->setProperty('publish_path');
    $this->setProperty('makefile', '');
    $this->setProperty('make_working_copy', FALSE);
  }

  /**
   * React to `provision-save` command by transforming publish_path into root.
   * This is necessary so that we don't conflict with drush's `--root`
   * option.
   */
  function save_platform() {
    $this->setProperty('root', $this->publish_path);
    $this->setProperty('publish_path', NULL);
    unset($this->publish_path);
  }
}

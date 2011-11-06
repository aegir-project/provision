<?php

/**
 * A basic configuration file class.
 *
 * Just a file containing the value passed to us.
 */
class Provision_Config_Example extends Provision_Config {
  /**
   * Template file to load. In the same directory as this class definition.
   */
  public $template = 'basic.tpl.php';

  /**
   * Where the file generated will end up.
   *
   * It is extremely important that this path is only made up of information
   * relative to this class, and does not use drush_get_option or the d() accessor.
   */
  function filename() {
    return $this->example_config_path . '/example.conf';
  }


  /**
   * Override the write method.
   */
  function write() {
    parent::write();

    // Sync the config to a remote server if necessary.
    $this->data['server']->sync($this->filename());
  }

  /**
   * Override the unlink method.
   */
  function unlink() {
    parent::unlink();

    // Remove the config from a remote server if necessary.
    $this->data['server']->sync($this->filename());
  }
}

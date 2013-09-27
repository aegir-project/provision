<?php

/**
 * Base class for subdir support.
 *
 * This class will publish the config files to remote
 * servers automatically.
 */
class Provision_Config_Apache_SubdirVhost extends Provision_Config_Http {
  public $template = 'subdir_vhost.tpl.php';
  public $description = 'subdirectory support';

  /**
   * Guess the URI this subdir alias is related too.
   */
  function uri() {
    return explode('/', $this->current_alias, 2)[0];
  }

  /**
   * Guess the subdir part of the subdir alias.
   */
  function subdir() {
    return explode('/', $this->current_alias, 2)[1];
  }

  function process() {
    parent::process();
    $this->data['uri'] = $this->uri();
    $this->data['subdir'] = $this->subdir();
    $this->data['subdirs_path'] = $this->data['http_subdird_path'] . '/' . $this->uri() . '/';
  }

  function filename() {
    return $this->data['http_subdird_path'] . '/' . $this->uri() . '/' . $this->subdir() . '.conf';
  }
}

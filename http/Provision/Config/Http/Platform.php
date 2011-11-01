<?php

/**
 * Base class for platform configuration files.
 */
class Provision_Config_Http_Platform extends Provision_Config_Http {
  public $template = 'platform.tpl.php';
  public $description = 'platform configuration file';

  function filename() {
    return $this->data['http_platformd_path'] . '/' . ltrim($this->context->name, '@') . '.conf';
  }
}

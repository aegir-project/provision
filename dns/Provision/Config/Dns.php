<?php

/**
 * Base config class for all dns config files.
 */
class Provision_Config_Dns extends Provision_Config {
  public $mode = 0777;
  function write() {
    parent::write();
    $this->data['server']->sync($this->filename());
  }

  function unlink() {
    $result = parent::unlink();
    $this->data['server']->sync($this->filename());
    return $result;
  }
}

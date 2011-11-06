<?php

/**
 * Base class for HTTP config files.
 *
 * This class will publish the config files to remote
 * servers automatically.
 */
class Provision_Config_Http extends Provision_Config {
  function write() {
    parent::write();
    $this->data['server']->sync($this->filename());
  }

  function unlink() {
    parent::unlink();
    $this->data['server']->sync($this->filename());
  }
}

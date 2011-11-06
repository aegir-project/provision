<?php

class Provision_Config_Global_Settings extends Provision_Config {
  public $template = 'global_settings.tpl.php';
  public $description = 'Global settings.php file';

  function filename() {
    return $this->include_path . '/global.inc';
  }
}

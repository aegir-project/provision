<?php

/**
 * Base class for subdir support.
 *
 * This class will publish the config files to remote
 * servers automatically.
 */
class Provision_Config_Apache_SubdirVhost extends Provision_Config_Http {
  public $template = 'subdir_vhost.tpl.php';
  public $description = 'subdirectory vhost support';

  // hack: because the parent class doesn't support multiple config
  // files, we need to keep track of the alias we're working on.
  protected $current_alias;

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

  function write() {
    foreach (d()->aliases as $alias) {
      if (strpos($alias, '/')) {
        $this->current_alias = $alias;
        drush_log("Subdirectory alias `$alias` found. Creating vhost configuration file.", 'notice');
        parent::write();
      }
    }
  }

  function process() {
    parent::process();
    $this->data['uri'] = $this->uri();
    $this->data['subdir'] = $this->subdir();
    $this->data['subdirs_path'] = $this->data['http_subdird_path'];
  }

  function filename() {
    // XXX: this will OVERWRITE existing vhosts!
    return $this->data['http_vhostd_path'] . '/' . $this->uri();
  }
}

<?php

/**
 * Base class for subdir support.
 *
 * This class will publish the config files to remote
 * servers automatically.
 */
class Provision_Config_Apache_Subdir extends Provision_Config_Http {
  public $template = 'subdir.tpl.php';
  public $disabled_template = 'subdir_disabled.tpl.php';
  public $description = 'subdirectory support';

  // hack: because the parent class doesn't support multiple config
  // files, we need to keep track of the alias we're working on.
  protected $current_alias;

  function write() {
    foreach (d()->aliases as $alias) {
      if (strpos($alias, '/')) {
        $this->current_alias = $alias;
        drush_log("Subdirectory alias `$alias` found. Creating configuration files.", 'notice');
        $uri_path = $this->data['http_subdird_path'] . '/' . $this->uri();
        provision_file()->create_dir($uri_path, dt("Webserver subdir configuration for domain"), 0700);
        $this->context->platform->server->sync($uri_path, array(
          'exclude' => $uri_path . '/*',  // Make sure remote directory is created
        ));
        parent::write();
      }
    }
  }

  /**
   * Guess the URI this subdir alias is related too.
   */
  function uri() {
    $e = explode('/', $this->current_alias, 2);
    return $e[0];
  }

  /**
   * Guess the subdir part of the subdir alias.
   */
  function subdir() {
    $e = explode('/', $this->current_alias, 2);
    return $e[1];
  }

  function process() {
    parent::process();
    $this->data['uri'] = $this->uri();
    $this->data['subdir'] = $this->subdir();
    if (!$this->site_enabled) {
      $this->template = $this->disabled_template;
    }
  }

  function filename() {
    return $this->data['http_subdird_path'] . '/' . $this->uri() . '/' . $this->subdir() . '.conf';
  }
}

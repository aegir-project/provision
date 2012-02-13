<?php
/**
 * @file
 * Provides the Provision_Config_Drushrc class.
 */

/**
 * Specialized class to handle the creation of drushrc.php files.
 *
 * This is based on the drush_save_config code, but has been abstracted
 * for our purposes.
 */
class Provision_Config_Drushrc extends Provision_Config {
  public $template = 'provision_drushrc.tpl.php';
  public $description = 'Drush configuration file';
  protected $mode = 0440;
  protected $context_name = 'drush';

  function filename() {
    return _drush_config_file($this->context_name);
  }

  function __construct($context, $data = array()) {
    parent::__construct($context, $data);
    $this->load_data();
  }

  function load_data() {
    // we fetch the context to pass into the template based on the context name
    $this->data = array_merge(drush_get_context($this->context_name), $this->data);
  }

  function process() {
    unset($this->data['context-path']);
    unset($this->data['config-file']);
    $this->data['option_keys'] = array_keys($this->data);
  }
}

<?php
/**
 * @file
 * Provides the Provision_Config_Drupal_Alias_Store class.
 */

class Provision_Config_Drupal_Alias_Store extends Provision_Config_Data_Store {
  public $template = 'provision_drupal_sites.tpl.php';
  public $description = 'Drupal sites.php file';
  public $key = 'sites';
  protected $mode = 0644;

  function filename() {
    return $this->root . '/sites/sites.php';
  }

  function maintain() {
    $this->delete();
    foreach ($this->aliases as $alias) {
      $this->records[$alias] = $this->uri;
    }
  }

  function delete() {
    foreach ($this->find() as $alias) {
      unset($this->records[$alias]);
      unset($this->loaded_records[$alias]);
    }
  }

  function find() {
    return array_keys($this->merged_records(), $this->uri);
  }
}

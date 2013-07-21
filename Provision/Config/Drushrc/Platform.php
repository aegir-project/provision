<?php
/**
 * @file
 * Provides the Provision_Config_Drushrc_Platform class.
 */

/**
 * Class for writing $platform/drushrc.php files.
 */
class Provision_Config_Drushrc_Platform extends Provision_Config_Drushrc {
  protected $context_name = 'drupal';
  public $description = 'Platform Drush configuration file';
  // platforms contain no confidential information
  protected $mode = 0444;

  function filename() {
    return $this->root . '/sites/all/drush/drushrc.php';
  }
}

<?php
/**
 * @file
 * Provides the Provision_Config_Drushrc_Server class.
 */

/**
 * Server level config for drushrc.php files.
 */
class Provision_Config_Drushrc_Server extends Provision_Config_Drushrc {
  protected $context_name = 'user';
  public $description = 'Server drush configuration';
}

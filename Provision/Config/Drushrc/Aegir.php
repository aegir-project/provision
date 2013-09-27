<?php
/**
 * @file
 * Provides the Provision_Config_Drushrc_Aegir class.
 */

/**
 * Class for writing the /var/aegir/.drush/drushrc.php file.
 */
class Provision_Config_Drushrc_Aegir extends Provision_Config_Drushrc {
  protected $context_name = 'home.drush';
  public $template = 'provision_drushrc_aegir.tpl.php';
  public $description = 'Aegir Drush configuration file';

  function __construct($context = '@none', $data = array()) {
    parent::__construct($context, $data);
    $this->load_data();
  }

  function load_data() {
    // List enabled Hosting Features.
    $features = hosting_get_features();
    foreach ($features as $name => $info) {
      if ($info['enabled'] == 1) {
        $features[$name] = $name;
      }
      else {
        unset($features[$name]);
      }
    }

    $this->data['hosting_features'] = $features;
  }

}

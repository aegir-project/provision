<?php

/**
 * Base config class for the server level config.
 */
class Provision_Config_Dns_Server extends Provision_Config_Dns {
  public $template = 'server.tpl.php';
  public $description = 'Server-wide DNS configuration';

  public $data_store_class = 'Provision_Config_Dns_Server_Store';

  function filename() {
    if (isset($this->data['application_name'])) {
      $file = $this->data['application_name'] . '.conf';
      return $this->data['server']->config_path . '/' . $file;
    }
    else {
      return FALSE;
    }
  }

  function write() {
    // lock the store until we are done generating our config.
    $this->store->lock();

    parent::write();

    $this->store->write();
    $this->store->close();
    if (isset($this->data['application_name'])) {
      $file = $this->data['application_name'] . '.conf';
      // We link the app_name.conf file on the remote server to the right version.
      $cmd = sprintf('ln -sf %s %s', 
        escapeshellarg($this->data['server']->config_path . '/' . $file), 
        escapeshellarg($this->data['server']->aegir_root . '/config/' . $file)
      );
      
      if ($this->data['server']->shell_exec($cmd)) {
        drush_log(dt("Created symlink for %file on %server", array(
          '%file' => $file,
          '%server' => $this->data['server']->remote_host,
        )));  
       
      };
    }
  }
}

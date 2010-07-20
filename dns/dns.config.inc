<?php


/**
 * Base config class for all dns config files.
 */
class provisionConfig_dns extends provisionConfig {
  function write() {
    parent::write();
    $this->data['server']->sync($this->filename());
  }

  function unlink() {
    parent::unlink();
    $this->data['server']->sync($this->filename());
  }
}

/**
 * Base config class for the server level config.
 */
class provisionConfig_dns_server extends provisionConfig_dns {
  public $template = 'server.tpl.php';
  public $description = 'Server-wide DNS configuration';

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
    parent::write();

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

class provisionConfig_dns_zone extends provisionConfig_dns {
  public $template = 'zone.tpl.php';
  public $description = 'Zone-wide DNS configuration';

  function filename() {
    return "{$this->data['server']->dns_zoned_path}/{$this->uri}.zone";
  }

}


class provisionConfig_dns_host extends provisionConfig_dns {
  public $template = 'host.tpl.php';
  public $description = 'Host-wide DNS configuration';

  function filename() {
    return "{$this->data['server']->dns_hostd_path}/{$this->uri}.hosts";
  }

}

<?php


/**
 * Base config class for all dns config files.
 */
class provisionConfig_dns extends provisionConfig {
  public $mode = 0777;
  function write() {
    parent::write();
    $this->data['server']->sync($this->filename());
  }

  function unlink() {
    parent::unlink();
    $this->data['server']->sync($this->filename());
  }
}


// The data store for the server configuration
// contains a list of zones we manage.
class provisionConfig_dns_server_store extends provisionConfig_data_store {
  function filename() {
    return $this->data['server']->dns_data_path . '/zones.master.inc';
  }
}

/**
 * Base config class for the server level config.
 */
class provisionConfig_dns_server extends provisionConfig_dns {
  public $template = 'server.tpl.php';
  public $description = 'Server-wide DNS configuration';

  public $data_store_class = 'provisionConfig_dns_server_store';

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

class provisionConfig_dns_zone_store extends provisionConfig_data_store {
  function filename() {
    return "{$this->data['server']->dns_data_path}/{$this->data['name']}.zone.inc";
  }
}

class provisionConfig_dns_zone extends provisionConfig_dns {
  public $template = 'zone.tpl.php';
  public $description = 'Zone-wide DNS configuration';

  public $data_store_class = 'provisionConfig_dns_zone_store';

  function filename() {
    return "{$this->data['server']->dns_zoned_path}/{$this->data['name']}.zone";
  }

  function process() {
    parent::process();
    $records = $this->store->merged_records();

    $this->data['dns_email'] = str_replace('@', '.', $this->data['server']->admin_email);

    // increment the serial.
    $this->store->records['@']['SOA']['serial'] = $records['serial'] = provisionService_dns::increment_serial($records['@']['SOA']['serial']);

    $this->data['records'] = $records;
  }

  function write() {
    // lock the store until we are done generating our config.
    $this->store->lock();

    parent::write();

    $this->store->write();
    $this->store->close();
  }

}


class provisionConfig_dns_host extends provisionConfig_dns {
  public $template = 'host.tpl.php';
  public $description = 'Host-wide DNS configuration';

  function filename() {
    return "{$this->data['server']->dns_hostd_path}/{$this->uri}.hosts";
  }
}



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

/**
 * Representation of a DNS zonefile
 *
 * This is the internal representation of a zonefile. It can be
 * extended by other subclasses to implement various engines, but it
 * has its own internal storage (through
 * provisionConfig_dns_zone_store).
 *
 * It assumes a certain structure in the records of the store.
 *
 * @example
 *
 * <code>
 * $zonefile = array('www' => array('a' => array('1.2.3.3', '1.2.3.4')),
 *                   '@' => array('SOA' => array('hostmaster' => 'localhost', 'email' => 'admin.localhost', 'serial' => '2010082301', 'refresh' => 21600 ... )'),
 *                               'A' => array('1.2.3.3'),
 *                               'MX' => array('mail.localhost'),
 *                               'NS' => array('localhost', 'ns2.localhost'),
 *                               )
 *                  );
 * </code>
 *
 * The zonefile's serial number is incremented automaticall when the
 * file is written (in process()). Note how the structure of the SOA
 * record is different from the others. First, it is a key-value
 * map. Second, it represents only one DNS record (whereas the other
 * entries represent as many entries as there are entries in the
 * array.
 *
 * To edit those records, some care need to be taken. Look at the
 * implementation of rr-add, rr-delete and rr-modify for examples of
 * how it should properly be done, in drush_dns_provision_zone().
 *
 * @see drush_dns_provision_zone()
 * @see increment_serial()
 * @see provisionConfig_dns_zone_store
 */
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
    $serial = (isset($records['@']['SOA']['serial']) ? $records['@']['SOA']['serial'] : NULL);
    $this->store->records['@']['SOA']['serial'] = $records['serial'] = provisionService_dns::increment_serial($serial);

    $this->data['records'] = $records;
  }

  function write() {
    // lock the store until we are done generating our config.
    $this->store->lock();

    if ($this->is_empty()) {
      $this->unlink();
    } else {
      parent::write();
      $this->store->write();
    }
    $this->store->close();
  }

  /**
   * this destroys this zonefile, without any checks
   *
   * It actually removes the zonefile, the internal storage and the
   * record in the server config.
   */
  function unlink() {
    // remove the zonefile
    if (parent::unlink()) {
      // remove the master record
      // XXX: need to do this for slaves too
      $this->server->service('dns')->config('server')->record_del($zone)->write();
      // remove the zonefile storage
      $this->store->unlink();
    }
    $this->store->unlock();
  }

  /**
   * test to see if the 
   */
  function is_empty() {
    $records = $this->store->merged_records();
    // if there is any record that is not SOA or NS, this is
    // considered empty
    if (empty($records)) {
      return TRUE;
    }
    foreach ($records as $name => $record) {
      if ($name != '@') {
        return FALSE;
      } else {
        foreach ($record as $type => $destination) {
          if ($type != 'SOA' && $type != 'NS' && !is_null($destination)) {
            return FALSE;
          }
        }
      }
    }
    return TRUE;
  }

}


class provisionConfig_dns_host extends provisionConfig_dns {
  public $template = 'host.tpl.php';
  public $description = 'Host-wide DNS configuration';

  function filename() {
    return "{$this->data['server']->dns_hostd_path}/{$this->uri}.hosts";
  }
}



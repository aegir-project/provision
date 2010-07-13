<?php

class provisionService_http_apache extends provisionService_http_public {
  protected $application_name = 'apache';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    return provisionService_http_apache::apache_restart_cmd();
  }

  function cloaked_db_creds() {
    return TRUE;
  }

  function init() {
    parent::init();
    $this->configs['server'][] = 'provisionConfig_apache_server';
    $this->configs['platform'][] = 'provisionConfig_apache_platform';
    $this->configs['site'][] = 'provisionConfig_apache_site';
  }

  public static function apache_restart_cmd() {
    $command = '/usr/sbin/apachectl'; // A proper default for most of the world
    foreach (explode(':', $_SERVER['PATH']) as $path) {
      $options[] = "$path/apache2ctl";
      $options[] = "$path/apachectl";
    }
    // Try to detect the apache restart command.
    $options[] = '/usr/local/sbin/apachectl'; // freebsd
    $options[] = '/usr/sbin/apache2ctl'; // debian + apache2
    $options[] = '/usr/apache2/2.2/bin'; // Solaris
    $options[] = $command;

    foreach ($options as $test) {
      if (is_executable($test)) {
        $command = $test;
        break;
      }
    }

    return "sudo $command graceful";
  }

  /**
   * Restart apache to pick up the new config files.
   */ 
  function parse_configs() {
    return $this->restart();
  }
}

class provisionConfig_apache extends provisionConfig {
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
 * Apache server level configuration file class
 */
class provisionConfig_apache_server extends provisionConfig_apache {
  public $template = 'server.tpl.php';
  public $description = 'apache server configuration file';

  function write() {
    parent::write();

    // We link the apache.config file on the remote server to the right version.
    $cmd = sprintf('ln -sf %s %s', 
      escapeshellarg($this->data['server']->config_path . '/apache.conf'), 
      escapeshellarg($this->data['server']->aegir_root . '/config/apache.conf')
    );

    $this->data['server']->shell_exec($cmd);
  }

  function filename() {
    return $this->data['server']->config_path . '/apache.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

/**
 * Apache platform level configuration file class
 */
class provisionConfig_apache_platform extends provisionConfig_apache {
  public $template = 'platform.tpl.php';
  public $description = 'apache platform configuration file';

  function filename() {
    return $this->data['http_platformd_path'] . '/' . ltrim($this->owner->name, '@') . '.conf';
  }

  function process() {
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_dir_config', $this->data));
  }
}

/** 
 * Apache site level config class. Virtual host.
 */
class provisionConfig_apache_site extends provisionConfig_apache {
  public $template = 'vhost.tpl.php';
  public $description = 'apache site configuration file';

  function filename() {
    return $this->data['http_vhostd_path'] . '/' . $this->uri;
  }

  function process() {
    if ($this->aliases && !is_array($this->aliases)) {
      $this->aliases = explode(",", $this->aliases);
    }

    if (!$this->site_enabled) {
      $this->template = 'vhost_disabled.tpl.php';
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->uri, $this->data));
  }
}


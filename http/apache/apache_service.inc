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

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'provisionConfig_apache_server';
    $this->configs['platform'][] = 'provisionConfig_apache_platform';
    $this->configs['site'][] = 'provisionConfig_apache_site';
  }

  /**
   * Guess at the likely value of the http_restart_cmd.
   *
   * This method is a static so that it can be re-used by the apache_ssl
   * service, even though it does not inherit this class.
   */
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

/**
 * Apache server level configuration file class
 */
class provisionConfig_apache_server extends provisionConfig_http_server {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $this->data));
  }
}

/**
 * Apache platform level configuration file class
 */
class provisionConfig_apache_platform extends provisionConfig_http_platform {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_dir_config', $this->data));
  }
}

/** 
 * Apache site level config class. Virtual host.
 */
class provisionConfig_apache_site extends provisionConfig_http_site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $this->uri, $this->data));
  }
}


<?php
/**
 * @file - nginx service for aegir.
 */

class provisionService_http_nginx extends provisionService_http_public {
  protected $application_name = 'nginx';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    return provisionService_http_nginx::nginx_restart_cmd();
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'provisionConfig_nginx_server';
    $this->configs['site'][] = 'provisionConfig_nginx_site';

    $this->server->setProperty('nginx_has_gzip', 0);
    $this->server->setProperty('nginx_has_new_version', 0);
    $this->server->setProperty('nginx_web_server', 0);
    $this->server->setProperty('nginx_has_upload_progress', 0);
  }

  function save_server() {
    // Check if some nginx features are supported and save them for later.
    $this->server->shell_exec('nginx -V');
    $this->server->nginx_has_gzip = preg_match("/(with-http_gzip_static_module)/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_upload_progress = preg_match("/(nginx-upload-progress-module)/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_new_version = preg_match("/(Barracuda\/0\.9\.)/", implode('', drush_shell_exec_output()), $match);
    $this->server->provision_db_cloaking = FALSE;
    $this->server->nginx_web_server = 1;
  }

  function verify_server_cmd() {
     provision_file()->copy(dirname(__FILE__) . '/nginx_advanced_include.conf', $this->server->include_path . '/nginx_advanced_include.conf');
     $this->sync($this->server->include_path . '/nginx_advanced_include.conf');
     provision_file()->copy(dirname(__FILE__) . '/nginx_simple_include.conf', $this->server->include_path . '/nginx_simple_include.conf');
     $this->sync($this->server->include_path . '/nginx_simple_include.conf');
     provision_file()->copy(dirname(__FILE__) . '/fastcgi_params.conf', $this->server->include_path . '/fastcgi_params.conf');
     $this->sync($this->server->include_path . '/fastcgi_params.conf');
     provision_file()->copy(dirname(__FILE__) . '/fastcgi_ssl_params.conf', $this->server->include_path . '/fastcgi_ssl_params.conf');
     $this->sync($this->server->include_path . '/fastcgi_ssl_params.conf');
    // Call the parent at the end. it will restart the server when it finishes.
    parent::verify_server_cmd();
  }

  /**
   * Guess at the likely value of the http_restart_cmd.
   *
   * This method is a static so that it can be re-used by the nginx_ssl
   * service, even though it does not inherit this class.
   */
  public static function nginx_restart_cmd() {
    $command = '/etc/init.d/nginx'; // A proper default for most of the world
    $options[] = $command;
    foreach (explode(':', $_SERVER['PATH']) as $path) {
      $options[] = "$path/nginx";
    }
    // Try to detect the nginx restart command.  
    $options[] = '/usr/sbin/nginx -s';
    $options[] = '/usr/local/sbin/nginx -s';
    $options[] = '/usr/local/bin/nginx -s';

    foreach ($options as $test) {
      if (is_executable($test)) {
        $command = $test;
        break;
      }
    }

    return "sudo $command reload";
  }

  /**
   * Restart/reload nginx to pick up the new config files.
   */ 
  function parse_configs() {
    return $this->restart();
  }
}

/**
 * Nginx server level configuration file class
 */
class provisionConfig_nginx_server extends provisionConfig_http_server {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_server_config', $this->data));
  }
}

/** 
 * Nginx site level config class. Virtual host.
 */
class provisionConfig_nginx_site extends provisionConfig_http_site {
  function process() {
    parent::process();
    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_nginx_vhost_config', $this->uri, $this->data));
  }
}


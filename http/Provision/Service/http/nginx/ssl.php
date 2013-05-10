<?php

/**
 * Nginx SSL service class.
 *
 * This class doesn't extend the nginx service itself, so there may
 * be some duplication of code between them. The majority of the
 * functionality is however implemented in the Provision_Service_http_public
 * class, which we do extend.
 */
class Provision_Service_http_nginx_ssl extends Provision_Service_http_ssl {
  // We share the application name with nginx.
  protected $application_name = 'nginx';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    // The nginx service defines it's restart command as a static
    // method so that we can make use of it here.
    return Provision_Service_http_nginx::nginx_restart_cmd();
  }

  public $ssl_enabled = TRUE;

  /**
   * Initialize the configuration files.
   *
   * These config classes are a mix of the SSL and Non-SSL nginx
   * classes. In some cases they extend the Nginx classes too.
   */
  function init_server() {
    parent::init_server();
    // Replace the server config with our own. See the class for more info.
    $this->configs['server'][] = 'Provision_Config_Nginx_Ssl_Server';
    $this->configs['site'][] = 'Provision_Config_Nginx_Ssl_Site';
    $this->server->setProperty('nginx_is_modern', 0);
    $this->server->setProperty('nginx_has_gzip', 0);
    $this->server->setProperty('basic_nginx_config', 0);
    $this->server->setProperty('extended_nginx_config', 0);
    $this->server->setProperty('provision_db_cloaking', FALSE);
  }

  function save_server() {
    // Find nginx executable.
    if (provision_file()->exists('/usr/local/sbin/nginx')->status()) {
      $path = "/usr/local/sbin/nginx";
    }
    elseif (provision_file()->exists('/usr/sbin/nginx')->status()) {
      $path = "/usr/sbin/nginx";
    }
    elseif (provision_file()->exists('/usr/local/bin/nginx')->status()) {
      $path = "/usr/local/bin/nginx";
    }
    else {
      return;
    }
    // Check if some nginx features are supported and save them for later.
    $this->server->shell_exec($path . ' -V');
    $this->server->nginx_is_modern = preg_match("/nginx\/1\.((1\.(8|9|(1[0-9]+)))|((2|3|4|5|6|7|8|9)\.))/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_gzip = preg_match("/http_gzip_static_module/", implode('', drush_shell_exec_output()), $match);

    // Use basic nginx configuration if this control file exists.
    $basic_nginx_config_file = "/etc/nginx/basic_nginx.conf";
    if (provision_file()->exists($basic_nginx_config_file)->status()) {
      $this->server->basic_nginx_config = 1;
      $this->server->extended_nginx_config = 0;
      drush_log(dt('Basic Nginx Config Active - YES control file found @path.', array('@path' => $basic_nginx_config_file)));
    }
    else {
      $this->server->basic_nginx_config = 0;
      $this->server->extended_nginx_config = 1;
      drush_log(dt('Extended Nginx Config Active - NO control file found @path.', array('@path' => $basic_nginx_config_file)));
    }
  }

  function verify_server_cmd() {
    // Find nginx executable.
    if (provision_file()->exists('/usr/local/sbin/nginx')->status()) {
      $path = "/usr/local/sbin/nginx";
    }
    elseif (provision_file()->exists('/usr/sbin/nginx')->status()) {
      $path = "/usr/sbin/nginx";
    }
    elseif (provision_file()->exists('/usr/local/bin/nginx')->status()) {
      $path = "/usr/local/bin/nginx";
    }
    else {
      return;
    }
    // Check if some nginx features are supported and save them for later.
    $this->server->shell_exec($path . ' -V');
    $this->server->nginx_is_modern = preg_match("/nginx\/1\.((1\.(8|9|(1[0-9]+)))|((2|3|4|5|6|7|8|9)\.))/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_gzip = preg_match("/http_gzip_static_module/", implode('', drush_shell_exec_output()), $match);

    // Use basic nginx configuration if this control file exists.
    $basic_nginx_config_file = "/etc/nginx/basic_nginx.conf";
    if (provision_file()->exists($basic_nginx_config_file)->status()) {
      $this->server->basic_nginx_config = 1;
      $this->server->extended_nginx_config = 0;
      drush_log(dt('Basic Nginx Config Active - YES control file found @path.', array('@path' => $basic_nginx_config_file)));
    }
    else {
      $this->server->basic_nginx_config = 0;
      $this->server->extended_nginx_config = 1;
      drush_log(dt('Extended Nginx Config Active - NO control file found @path.', array('@path' => $basic_nginx_config_file)));
    }

    provision_file()->copy(dirname(dirname(__FILE__)) . '/nginx_advanced_include.conf', $this->server->include_path . '/nginx_advanced_include.conf');
    $this->sync($this->server->include_path . '/nginx_advanced_include.conf');
    provision_file()->copy(dirname(dirname(__FILE__)) . '/nginx_simple_include.conf', $this->server->include_path . '/nginx_simple_include.conf');
    $this->sync($this->server->include_path . '/nginx_simple_include.conf');
    // Call the parent at the end. it will restart the server when it finishes.
    parent::verify_server_cmd();
  }

  /**
   * Restart/reload nginx to pick up the new config files.
   */
  function parse_configs() {
    return $this->restart();
  }
}

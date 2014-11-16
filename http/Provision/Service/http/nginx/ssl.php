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

  function cloaked_db_creds() {
    return TRUE;
  }

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
    $this->server->setProperty('nginx_config_mode', 'extended');
    $this->server->setProperty('nginx_is_modern', FALSE);
    $this->server->setProperty('nginx_has_gzip', FALSE);
    $this->server->setProperty('nginx_has_upload_progress', FALSE);
    $this->server->setProperty('provision_db_cloaking', TRUE);
    $this->server->setProperty('phpfpm_mode', 'port');
    $this->server->setProperty('satellite_mode', 'vanilla');
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
    $this->server->nginx_has_upload_progress = preg_match("/upload/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_gzip = preg_match("/http_gzip_static_module/", implode('', drush_shell_exec_output()), $match);

    // Use basic nginx configuration if this control file exists.
    $nginx_config_mode_file = "/etc/nginx/basic_nginx.conf";
    if (provision_file()->exists($nginx_config_mode_file)->status()) {
      $this->server->nginx_config_mode = 'basic';
      drush_log(dt('Basic Nginx Config Active -SAVE- YES control file found @path.', array('@path' => $nginx_config_mode_file)));
    }
    else {
      $this->server->nginx_config_mode = 'extended';
      drush_log(dt('Extended Nginx Config Active -SAVE- NO control file found @path.', array('@path' => $nginx_config_mode_file)));
    }

    // Check if there is php-fpm listening on unix socket, otherwise use port 9000 to connect
    if (provision_file()->exists('/var/run/php5-fpm.sock')->status()) {
      $this->server->phpfpm_mode = 'socket';
      drush_log(dt('PHP-FPM unix socket mode detected -SAVE- YES socket found @path.', array('@path' => '/var/run/php5-fpm.sock')));
    }
    else {
      $this->server->phpfpm_mode = 'port';
      drush_log(dt('PHP-FPM port mode detected -SAVE- NO socket found @path.', array('@path' => '/var/run/php5-fpm.sock')));
    }

    // Check if there is BOA specific global.inc file to enable extra Nginx locations
    if (provision_file()->exists('/data/conf/global.inc')->status()) {
      $this->server->satellite_mode = 'boa';
      drush_log(dt('BOA mode detected -SAVE- YES file found @path.', array('@path' => '/data/conf/global.inc')));
    }
    else {
      $this->server->satellite_mode = 'vanilla';
      drush_log(dt('Vanilla mode detected -SAVE- NO file found @path.', array('@path' => '/data/conf/global.inc')));
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
    $this->server->nginx_has_upload_progress = preg_match("/upload/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_gzip = preg_match("/http_gzip_static_module/", implode('', drush_shell_exec_output()), $match);

    // Use basic nginx configuration if this control file exists.
    $nginx_config_mode_file = "/etc/nginx/basic_nginx.conf";
    if (provision_file()->exists($nginx_config_mode_file)->status()) {
      $this->server->nginx_config_mode = 'basic';
      drush_log(dt('Basic Nginx Config Active -VERIFY- YES control file found @path.', array('@path' => $nginx_config_mode_file)));
    }
    else {
      $this->server->nginx_config_mode = 'extended';
      drush_log(dt('Extended Nginx Config Active -VERIFY- NO control file found @path.', array('@path' => $nginx_config_mode_file)));
    }

    // Check if there is php-fpm listening on unix socket, otherwise use port 9000 to connect
    if (provision_file()->exists('/var/run/php5-fpm.sock')->status()) {
      $this->server->phpfpm_mode = 'socket';
      drush_log(dt('PHP-FPM unix socket mode detected -VERIFY- YES socket found @path.', array('@path' => '/var/run/php5-fpm.sock')));
    }
    else {
      $this->server->phpfpm_mode = 'port';
      drush_log(dt('PHP-FPM port mode detected -VERIFY- NO socket found @path.', array('@path' => '/var/run/php5-fpm.sock')));
    }

    // Check if there is BOA specific global.inc file to enable extra Nginx locations
    if (provision_file()->exists('/data/conf/global.inc')->status()) {
      $this->server->satellite_mode = 'boa';
      drush_log(dt('BOA mode detected -VERIFY- YES file found @path.', array('@path' => '/data/conf/global.inc')));
    }
    else {
      $this->server->satellite_mode = 'vanilla';
      drush_log(dt('Vanilla mode detected -VERIFY- NO file found @path.', array('@path' => '/data/conf/global.inc')));
    }

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

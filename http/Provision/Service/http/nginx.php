<?php

class Provision_Service_http_nginx extends Provision_Service_http_public {
  protected $application_name = 'nginx';
  protected $has_restart_cmd = TRUE;

  function default_restart_cmd() {
    return Provision_Service_http_nginx::nginx_restart_cmd();
  }

  function cloaked_db_creds() {
    return TRUE;
  }

  function init_server() {
    parent::init_server();
    $this->configs['server'][] = 'Provision_Config_Nginx_Server';
    $this->configs['server'][] = 'Provision_Config_Nginx_Inc_Server';
    $this->configs['site'][] = 'Provision_Config_Nginx_Site';
    $this->server->setProperty('nginx_config_mode', 'extended');
    $this->server->setProperty('nginx_is_modern', FALSE);
    $this->server->setProperty('nginx_has_gzip', FALSE);
    $this->server->setProperty('nginx_has_upload_progress', FALSE);
    $this->server->setProperty('provision_db_cloaking', TRUE);
    $this->server->setProperty('phpfpm_mode', 'port');
    $this->server->setProperty('subdirs_support', FALSE);
    $this->server->setProperty('satellite_mode', 'vanilla');
    if (provision_hosting_feature_enabled('subdirs')) {
      $this->server->subdirs_support = TRUE;
      $this->configs['site'][] = 'Provision_Config_Nginx_Subdir';
      $this->configs['site'][] = 'Provision_Config_Nginx_SubdirVhost';
    }
  }

  function save_server() {

    // Set correct provision_db_cloaking value on server save.
    $this->server->provision_db_cloaking = TRUE;

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

    // Set correct subdirs_support value on server save
    if (provision_hosting_feature_enabled('subdirs')) {
      $this->server->subdirs_support = TRUE;
    }
  }

  function verify_server_cmd() {

    // Set correct provision_db_cloaking value on server verify.
    $this->server->provision_db_cloaking = TRUE;

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

    // Set correct subdirs_support value on server verify
    if (provision_hosting_feature_enabled('subdirs')) {
      $this->server->subdirs_support = TRUE;
    }

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
    // Try to detect the nginx restart command.
    foreach (explode(':', $_SERVER['PATH']) as $path) {
      $options[] = "$path/nginx";
    }
    $options[] = '/usr/sbin/nginx';
    $options[] = '/usr/local/sbin/nginx';
    $options[] = '/usr/local/bin/nginx';

    foreach ($options as $test) {
      if (is_executable($test)) {
        $command = ($test == '/etc/init.d/nginx') ? $test : $test . ' -s';
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

<?php

class Provision_Service_http_nginx extends Provision_Service_http_public {

  // Define static socket file locations for various PHP versions.
  // These are dynamic in PHP 7.
  const SOCKET_PATH_PHP5 = '/var/run/php5-fpm.sock';
  const SOCKET_PATH_PHP7_BASE = '/var/run/php';

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
    $this->server->setProperty('nginx_has_etag', FALSE);
    $this->server->setProperty('nginx_has_http2', FALSE);
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
    $this->server->nginx_is_modern = preg_match("/nginx\/1\.((1\.(8|9|(1[0-9]+)))|((2|3|4|5|6|7|8|9|[1-9][0-9]+)\.))/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_etag = preg_match("/nginx\/1\.([12][0-9]|[3]\.([12][0-9]|[3-9]))/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_http2 = preg_match("/http_v2_module/", implode('', drush_shell_exec_output()), $match);
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
    $this->server->phpfpm_mode = $this->getPhpFpmMode('save');

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
    $this->server->nginx_is_modern = preg_match("/nginx\/1\.((1\.(8|9|(1[0-9]+)))|((2|3|4|5|6|7|8|9|[1-9][0-9]+)\.))/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_etag = preg_match("/nginx\/1\.([12][0-9]|[3]\.([12][0-9]|[3-9]))/", implode('', drush_shell_exec_output()), $match);
    $this->server->nginx_has_http2 = preg_match("/http_v2_module/", implode('', drush_shell_exec_output()), $match);
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
    $this->server->phpfpm_mode = $this->getPhpFpmMode('verify');

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
   * Determines the PHP FPM mode.
   *
   * @param string $server_task
   *   The server task type for logging purposes. Leave blank to skip logging.
   * @return string
   *   The mode, either 'socket' or 'port'.
   */
  public static function getPhpFpmMode($server_task = NULL) {

    // Search for socket files or fall back to port mode.
    switch (TRUE) {
      case provision_file()->exists(self::SOCKET_PATH_PHP5)->status():
        $mode = 'socket';
        $socket_path = self::SOCKET_PATH_PHP5;
      break;
      case provision_file()->exists(static::getPhp7FpmSocketPath())->status():
        $mode = 'socket';
        $socket_path = static::getPhp7FpmSocketPath();
      break;
      default:
        $mode = 'port';
        $socket_path = '';
      break;
    }

    // Report results in the log if requested.
    if (!empty($server_task)) {
      drush_log(dt('PHP-FPM @mode mode detected -' . '@task' . '- @yes_or_no socket found @path.', array(
        '@mode' => ($mode == 'socket') ? 'unix socket' : 'port',
        '@task' => strtoupper($server_task),
        '@yes_or_no' => ($mode == 'socket') ? 'YES' : 'NO',
        '@path' => ($socket_path ? $socket_path : self::SOCKET_PATH_PHP5 . ' or ' . static::getPhp7FpmSocketPath()),
      )));
    }

    // Return the discovered mode.
    return $mode;
  }

  /**
   * Gets the PHP FPM unix socket path.
   *
   * If we're running in port mode, there is no socket path. FALSE would be
   * returned in this case.
   *
   * @return string
   *   The path, or FALSE if there isn't one.
   */
  public static function getPhpFpmSocketPath() {
    // Simply return FALSE if we're in port mode.
    if (self::getPhpFpmMode() == 'port') {
      return FALSE;
    }

    // Return the socket path based on the PHP version.
    if (strtok(phpversion(), '.') == 7) {
      return static::getPhp7FpmSocketPath();
    }
    else {
      return self::SOCKET_PATH_PHP5;
    }
  }

  /**
   * Gets the PHP FPM unix socket path for PHP 7.
   *
   * In PHP 7, there isn't a fixed socked path.  It could be any one of the
   * following:
   *   * php7.0-fpm.sock
   *   * php7.2-fpm.sock
   *   * ...
   *
   * @return string
   *   The path, or FALSE if there isn't one.
   */
  public static function getPhp7FpmSocketPath() {
    foreach (scandir(static::SOCKET_PATH_PHP7_BASE, SCANDIR_SORT_DESCENDING) as $file) {
      if (strpos($file, 'fpm.sock')) {
        return static::SOCKET_PATH_PHP7_BASE . '/' . $file;
      }
    }
    return FALSE;
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

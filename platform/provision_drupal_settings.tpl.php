<?php print '<?php' ?>
  /**
   * The database credentials are stored in the Apache vhost config
   * of the associated site with SetEnv parameters.
   * They are called here with $_SERVER environment variables to 
   * prevent sensitive data from leaking to site administrators 
   * with PHP access, that potentially might be of other sites in
   * Drupal's multisite set-up.
   * This is a security measure implemented by the Aegir project.
   */
  $databases['default']['default'] = array(
    'driver' => urldecode($_SERVER['db_type']),
    'database' => urldecode($_SERVER['db_name']),
    'username' => urldecode($_SERVER['db_user']),
    'password' => urldecode($_SERVER['db_passwd']),
    'host' => urldecode($_SERVER['db_host']),
  );
  $db_url = "<?php print strtr("%db_type://%db_user:%db_passwd@%db_host/%db_name", array(
    '%db_type' => '$_SERVER[db_type]',
    '%db_user' => '$_SERVER[db_user]', 
    '%db_passwd' => '$_SERVER[db_passwd]',
    '%db_host' => '$_SERVER[db_host]', 
    '%db_name' => '$_SERVER[db_name]')); ?>";


  $profile = "<?php print $this->profile ?>";
  /**
  * PHP settings:
  *
  * To see what PHP settings are possible, including whether they can
  * be set at runtime (ie., when ini_set() occurs), read the PHP
  * documentation at http://www.php.net/manual/en/ini.php#ini.list
  * and take a look at the .htaccess file to see which non-runtime
  * settings are used there. Settings defined here should not be
  * duplicated there so as to avoid conflict issues.
  */
  @ini_set('arg_separator.output',     '&amp;');
  @ini_set('magic_quotes_runtime',     0);
  @ini_set('magic_quotes_sybase',      0);
  @ini_set('session.cache_expire',     200000);
  @ini_set('session.cache_limiter',    'none');
  @ini_set('session.cookie_lifetime',  0);
  @ini_set('session.gc_maxlifetime',   200000);
  @ini_set('session.save_handler',     'user');
  @ini_set('session.use_only_cookies', 1);
  @ini_set('session.use_trans_sid',    0);
  @ini_set('url_rewriter.tags',        '');

  /**
  * Set the umask so that new directories created by Drupal have the correct permissions
  */
  umask(0002);


  global $conf;
  $conf['install_profile'] = '<?php print $this->profile ?>';
  $conf['file_directory_path'] = 'sites/<?php print $this->uri ?>/files';
  $conf['file_directory_temp'] = 'sites/<?php print $this->uri ?>/files/tmp';
  $conf['file_downloads'] = 1;
  $conf['cache'] = 1;
  $conf['clean_url'] = 1;

  <?php if (!$this->site_enabled) : ?>
    $conf['site_offline'] = 1;
  <?php endif ?>

<?php print $extra_config; ?>

  /**
  * This was added from Drupal 5.2 onwards.
  */
  /**
  * We try to set the correct cookie domain. If you are experiencing problems
  * try commenting out the code below or specifying the cookie domain by hand.
  */
  if (isset($_SERVER['HTTP_HOST'])) {
    $domain = '.'. preg_replace('`^www.`', '', $_SERVER['HTTP_HOST']);
    // Per RFC 2109, cookie domains must contain at least one dot other than the
    // first. For hosts such as 'localhost', we don't set a cookie domain.
    if (count(explode('.', $domain)) > 2) {
      @ini_set('session.cookie_domain', $domain);
    }
  }

  # Additional host wide configuration settings. Useful for safely specifying configuration settings.
  if (file_exists('<?php print $this->platform->server->include_path  ?>/global.inc')) {
    include_once('<?php print $this->platform->server->include_path  ?>/global.inc');
  }

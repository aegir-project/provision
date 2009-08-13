
  $databases['default']['default'] = array(
    'driver' => '<?php print $db_type; ?>',
    'database' => '<?php print $db_name; ?>',
    'username' => '<?php print $db_user; ?>',
    'password' => '<?php print $db_passwd; ?>',
    'host' => '<?php print $db_host ?>',
  );
  $db_url = '<?php print strtr("%db_type://%db_user:%db_passwd@%db_host/%db_name", array('%db_type' => urlencode($db_type), '%db_user' => urlencode($db_user), '%db_passwd' => urlencode($db_passwd), '%db_host' => urlencode($db_host), '%db_name' => urlencode($db_name))); ?>';

  $profile = "<?php print $profile ?>";
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
  $conf['file_directory_path'] = conf_path() . '/files';
  $conf['file_directory_temp'] = conf_path() . '/files/tmp';
  $conf['file_downloads'] = 1;
  $conf['cache'] = 1;
  $conf['clean_url'] = 1;

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
  if (file_exists('<?php print $config_path  ?>/includes/global.inc')) {
    include_once('<?php print $config_path  ?>/includes/global.inc');
  }

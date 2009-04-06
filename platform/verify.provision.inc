<?php

/**
 * Provision verify command
 *
 * Disable an enabled and installed site
 */
function provision_drupal_provision_verify_validate($url = null) {
  if ($url) {
    _provision_errors_on();
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  }

  // This will not have an effect if you are running it on a platform, just tests that if a site is provided it is an existing one
  _provision_drupal_valid_site();
}


/**
 * implementation of provision_verify
 */
function provision_drupal_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    _provision_create_dir(drush_get_option('config_path'), dt('Provision configuration'), 0700);
    _provision_create_dir(drush_get_option('backup_path'), dt('Web server configuration'), 0700);
    provision_path("writable", drush_get_option('sites_path'), TRUE, dt("Drupal sites directory is writable by the provisioning script"),
      dt("Drupal sites directory is not writable by the provisioning script"), PROVISION_SITES_DIR_NOT_WRITABLE);

    drush_set_option('sites', array_keys((array) provision_drupal_find_sites()), 'drupal'); 
    drush_log(dt("This platform is running @short_name @version", array('@short_name' => 'drupal', '@version' => VERSION)));
    drush_set_option('packages', _scrub_object(provision_find_packages()), 'drupal');
  }
  else {
    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
      // This is the actual drupal provisioning requirements. 
    _provision_drupal_create_directories($url);
    _provision_drupal_maintain_aliases($url);
    // Requires at least the database settings to complete.
   _provision_drupal_create_settings_file($url);
  }

}


/**
 * Implementation of hook_provision_post_verify
 */
function provision_drupal_post_provision_verify($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_drupal_rebuild_caches($url);
    drush_set_option('installed', TRUE, 'site');
  }
}

/**
 * Inject the relevant .htacces configuration into the global apache configuration
 */
function provision_drupal_provision_apache_dir_config($data = null) {
  return <<<EOF
#
# Apache/PHP/Drupal settings:
#

# Protect files and directories from prying eyes.
<FilesMatch "\.(engine|inc|info|install|module|profile|test|po|sh|.*sql|theme|tpl(\.php)?|xtmpl|svn-base)$|^(code-style\.pl|Entries.*|Repository|Root|Tag|Template|all-wcprops|entries|format)$">
  Order allow,deny
</FilesMatch>

# Don't show directory listings for URLs which map to a directory.
Options -Indexes

# Follow symbolic links in this directory.
Options +FollowSymLinks

# Make Drupal handle any 404 errors.
ErrorDocument 404 /index.php

# Force simple error message for requests for non-existent favicon.ico.
<Files favicon.ico>
  # There is no end quote below, for compatibility with Apache 1.3.
  ErrorDocument 404 "The requested file favicon.ico was not found.
</Files>

# Set the default handler.
DirectoryIndex index.php

# Override PHP settings. More in sites/default/settings.php
# but the following cannot be changed at runtime.

# PHP 4, Apache 1.
<IfModule mod_php4.c>
  php_value magic_quotes_gpc                0
  php_value register_globals                0
  php_value session.auto_start              0
  php_value mbstring.http_input             pass
  php_value mbstring.http_output            pass
  php_value mbstring.encoding_translation   0
</IfModule>

# PHP 4, Apache 2.
<IfModule sapi_apache2.c>
  php_value magic_quotes_gpc                0
  php_value register_globals                0
  php_value session.auto_start              0
  php_value mbstring.http_input             pass
  php_value mbstring.http_output            pass
  php_value mbstring.encoding_translation   0
</IfModule>

# PHP 5, Apache 1 and 2.
<IfModule mod_php5.c>
  php_value magic_quotes_gpc                0
  php_value register_globals                0
  php_value session.auto_start              0
  php_value mbstring.http_input             pass
  php_value mbstring.http_output            pass
  php_value mbstring.encoding_translation   0
</IfModule>

# Requires mod_expires to be enabled.
<IfModule mod_expires.c>
  # Enable expirations.
  ExpiresActive On

  # Cache all files for 2 weeks after access (A).
  ExpiresDefault A1209600

  # Do not cache dynamically generated pages.
  ExpiresByType text/html A1
</IfModule>

# Various rewrite rules.
<IfModule mod_rewrite.c>
  RewriteEngine on

  # Rewrite URLs of the form 'x' to the form 'index.php?q=x'.
  RewriteCond %{REQUEST_FILENAME} !-f
  RewriteCond %{REQUEST_FILENAME} !-d
  RewriteCond %{REQUEST_URI} !=/favicon.ico
  RewriteRule ^(.*)$ index.php?q=$1 [L,QSA]
</IfModule>

# Do not read the platform's .htaccess
AllowOverride none

EOF;
}

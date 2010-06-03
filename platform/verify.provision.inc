<?php

/**
 * Provision verify command
 *
 * Disable an enabled and installed site
 */
function drush_provision_drupal_provision_verify_validate() {
  if (PROVISION_CONTEXT_SITE) {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
    provision_prepare_environment();

    _provision_drupal_create_settings_file();
    drush_errors_on();
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  }

  // This will not have an effect if you are running it on a platform, just
  // tests that if a site is provided it is an existing one
  _provision_drupal_valid_site();
}


/**
 * implementation of provision_verify
 */
function drush_provision_drupal_provision_verify() {
  if (PROVISION_CONTEXT_SERVER) {
    provision_service('file')->create_dir(d()->config_path, dt('Provision configuration'), 0711);
    provision_service('file')->create_dir(d()->config_path . '/includes', dt('Provision PHP configuration'), 0711);
    if (!provision_service('file')->exists(d()->config_path . '/includes/global.inc')->succeed('Global configuration file exists')->status()) {
      // Create an empty global.inc so the include doesn't fail with
      // open_basedir restrictions
      $config = new provisionConfig_global_settings(d()->name);
      $config->write();
    }
    provision_service('file')->create_dir(d()->backup_path, dt('Backup'), 0700);
  }
  elseif (PROVISION_CONTEXT_PLATFORM) {
    provision_service('file')->writable(d()->root . '/sites')
      ->succeed('Drupal sites directory @path is writable by the provisioning script')
      ->fail('Drupal sites directory @path is not writable by the provisioning script', 'PROVISION_SITES_DIR_NOT_WRITABLE');

    drush_set_option('sites', array_keys((array) provision_drupal_find_sites()), 'drupal'); 
    drush_log(dt("This platform is running @short_name @version", array('@short_name' => 'drupal', '@version' => VERSION)));
    drush_set_option('packages', _scrub_object(provision_find_packages()), 'drupal');
    _provision_drupal_create_directories('all');
  }
  elseif (PROVISION_CONTEXT_SITE) {
    drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
    // This is the actual drupal provisioning requirements. 
    _provision_drupal_create_directories();
    _provision_drupal_maintain_aliases();
    // Requires at least the database settings to complete.
   _provision_drupal_create_settings_file();
  }
}

class provisionConfig_global_settings extends provisionConfig {
  public $template = 'global_settings.tpl.php';
  public $description = 'Global settings.php file';

  function filename() {
    return $this->config_path . '/includes/global.inc';
  }

  function process() {
  }
}

/**
 * Implementation of hook_provision_post_verify
 */
function drush_provision_drupal_post_provision_verify() {
  if (PROVISION_CONTEXT_SITE) {
    _provision_drupal_rebuild_caches();
    drush_set_option('aliases', drush_get_option('aliases'), 'site');
    drush_set_option('redirection', drush_get_option('redirection'), 'site');
    drush_set_option('installed', TRUE, 'site');
  }
}

/**
 * Inject the relevant .htacces configuration into the global apache
 * configuration
 *
 * This basically loads the regular .htacess but adds an AllowOverride
 * none so the .htaccess are ignored, as a performance improvement.
 *
 * @see platform/provision_drupal_htaccess.tpl.php
 * @see hook_provision_apache_dir_config()
 */
function provision_drupal_provision_apache_dir_config($data = null) {
  $htaccess = file_get_contents(drush_get_option('root') . '/.htaccess');
  $htaccess .= <<<EOF

<IfModule mod_rewrite.c>
  # allow files to be accessed without /sites/fqdn/
  RewriteRule ^files/(.*)$ /sites/%{HTTP_HOST}/files/$1 [L]
</IfModule>

# Do not read the platform's .htaccess
AllowOverride none

EOF;
  # the trailing newline above is necessary
  return $htaccess;
}

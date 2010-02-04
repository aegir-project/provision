<?php

/**
 * Provision verify command
 *
 * Disable an enabled and installed site
 */
function drush_provision_drupal_provision_verify_validate($url = null) {
  if ($url) {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);
    provision_prepare_environment();

      _provision_drupal_create_settings_file($url);
     drush_errors_on();
     drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  }

  // This will not have an effect if you are running it on a platform, just tests that if a site is provided it is an existing one
  _provision_drupal_valid_site();
}


/**
 * implementation of provision_verify
 */
function drush_provision_drupal_provision_verify($url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    _provision_create_dir(drush_get_option('config_path'), dt('Provision configuration'), 0711);
    _provision_create_dir(drush_get_option('config_path') . '/includes', dt('Provision PHP configuration'), 0711);
    if (!provision_path('exists', drush_get_option('config_path') .  '/includes/global.inc', TRUE, dt("Global configuration file exists"))) {
      # create an empty global.inc so the include doesn't fail with
      # open_basedir restrictions
      if (!$file = fopen(drush_get_option('config_path') .  '/includes/global.inc', 'a')) {
        drush_set_error('PROVISION_FRAMEWORK_ERROR', dt('Cannot create global settings configuration'));
      } else {
        fwrite($file, "<?php # global settings.php");
        fclose($file);
      }
    }
    _provision_create_dir(drush_get_option('backup_path'), dt('Backup'), 0700);
    provision_path("writable", drush_get_option('sites_path'), TRUE, dt("Drupal sites directory is writable by the provisioning script"),
      dt("Drupal sites directory is not writable by the provisioning script"), 'PROVISION_SITES_DIR_NOT_WRITABLE');

    drush_set_option('sites', array_keys((array) provision_drupal_find_sites()), 'drupal'); 
    drush_log(dt("This platform is running @short_name @version", array('@short_name' => 'drupal', '@version' => VERSION)));
    drush_set_option('packages', _scrub_object(provision_find_packages()), 'drupal');
    _provision_drupal_create_directories('all');
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
function drush_provision_drupal_post_provision_verify($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_drupal_rebuild_caches($url);
    drush_set_option('aliases', drush_get_option('aliases'), 'site');
    drush_set_option('redirection', drush_get_option('redirection'), 'site');
    drush_set_option('installed', TRUE, 'site');
  }
}

/**
 * Inject the relevant .htacces configuration into the global apache configuration
 */
function provision_drupal_provision_apache_dir_config($data = null) {
  $htaccess = file_get_contents(dirname(__FILE__) . "/provision_drupal_htaccess.tpl.php");
  $htaccess .= <<<EOF
# Do not read the platform's .htaccess
AllowOverride none
EOF;
  return $htaccess;
}

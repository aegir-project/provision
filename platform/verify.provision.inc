<?php

/**
 * Provision verify command
 *
 * Disable an enabled and installed site
 */
function provision_drupal_provision_verify_validate() {
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

    drush_set_option('sites', array_keys((array) provision_drupal_find_sites()), 'platform'); 
    drush_set_option('platform', array('short_name' => 'drupal', 'version' => drush_drupal_version()), 'platform'); 
    print_r(_drush_get_context());
    drush_log(dt("This platform is running @short_name @version", 
      array('@short_name' => 'drupal', '@version' => VERSION)));
    drush_set_option('profiles', _scrub_object(_provision_find_profiles()), 'platform');

  }
  else {
      // This is the actual drupal provisioning requirements. 
    _provision_drupal_create_directories($url);
    _provision_drupal_maintain_aliases($url);
    // Requires at least the database settings to complete.
   _provision_drupal_create_settings_file($url);
    provision_internal_init($url);
    provision_platform_include(dirname(__FILE__), 'verify');
  }
  #if (is_array($data['modules'])) {
    // get the correct version names for everything.
    #$data['modules'] = _provision_drupal_get_cvs_versions($data['modules']);
  #}
}


/**
 * Implementation of hook_provision_post_verify
 */
function provision_drupal_provision_post_verify($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_drupal_rebuild_caches($url);
    drush_set_option('installed', TRUE, 'site');
  }
}


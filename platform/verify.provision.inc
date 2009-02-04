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
function provision_drupal_provision_verify(&$data, $url = null) {
  if (PROVISION_CONTEXT_PLATFORM) {
    _provision_create_dir(PROVISION_CONFIG_PATH, dt('Provision configuration'), 0700);
    _provision_create_dir(PROVISION_BACKUP_PATH, dt('Web server configuration'), 0700);
    provision_path("writable", "sites", TRUE, dt("Drupal sites directory is writable by the provisioning script"),
      dt("Drupal sites directory is not writable by the provisioning script"), PROVISION_PERM_ERROR);
    $data['sites'] = array_keys((array) provision_drupal_find_sites()); // return list of hosted sites. used to determine whether or not to import. 

    $data['platform'] = array('short_name' => 'drupal', 'version' => drush_drupal_version());  
    provision_log('notice', dt("This platform is running @short_name @version", 
      array('@short_name' => 'drupal', '@version' => VERSION)));
    $data['profiles'] = _provision_find_profiles();
  }
  else {
      // This is the actual drupal provisioning requirements. 
    _provision_drupal_create_directories($url, $data['profile']);
    _provision_drupal_maintain_aliases($data, $url);
    // Requires at least the database settings to complete.
    _provision_drupal_create_settings_file($data, $url);
//    provision_internal_init($url);
//    provision_platform_include(dirname(__FILE__), 'verify');
  }
  if (is_array($data['modules'])) {
    // get the correct version names for everything.
    #$data['modules'] = _provision_drupal_get_cvs_versions($data['modules']);
  }
}


/**
 * Implementation of hook_provision_post_verify
 */
function provision_drupal_provision_post_verify(&$data, $url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_drupal_rebuild_caches($data, $url);
    $data['installed'] = TRUE;
  }
}


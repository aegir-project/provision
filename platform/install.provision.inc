<?php
/**
 * @file
 *   Provision hooks for the provision install command.
 */

/**
 * Provision install command
 *
 * These are the hooks that will be executed by the provision_invoke function
 * when doing a provision_install.
 */

/**
 * Check that we are trying to install a new site , and a new site only
 */
function provision_drupal_provision_install_validate() {
  _provision_drupal_url_required();
  _provision_drupal_valid_not_installed_site();
}

/**
 * Set up the directories and settings.php file that we need.
 */
function provision_drupal_provision_pre_install(&$data, $url = NULL) {
 // This is the actual drupal provisioning requirements. 
 _provision_drupal_create_directories($url, $data['profile']);
}

/**
 * Install Drupal with the pre-configured settings, by calling an external script
 *
 * This is an external script so that php is running in it's own namespace, and
 * weird problems such as the multiple database connections don't confuse drupal.
 */
function provision_drupal_provision_install(&$data, $url = NULL) {
 // Requires at least the database settings to complete.
  _provision_drupal_create_settings_file($data, $url);
  provision_internal_init($url, FALSE);
  provision_platform_include(dirname(__FILE__), 'install');
}

/**
 * If the install went south, and the site is not PROVISION_SITE_INSTALLED, clean up behind ourselves
 */
function provision_drupal_provision_install_rollback(&$data, $url = NULL) {
  if (!(provision_get_error() & PROVISION_SITE_INSTALLED)) {
    _provision_recursive_delete("sites/$url");
  }
}


/**
 * Finish the installation, regenerate the caches on the site so that
 * any changes to things such as available modules/ themes can take affect.
 */
function provision_drupal_provision_post_install(&$data, $url = NULL) {
  $data['installed'] = TRUE;
  _provision_drupal_maintain_aliases($url, $data);
  provision_path("chmod", "./sites/$url/settings.php", 0440, dt("Secured settings.php with safe permissions"));
  _provision_drupal_rebuild_caches($data, $url);
}


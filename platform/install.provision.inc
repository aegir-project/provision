<?php
/**
 * @file
 *   Provision hooks for the provision install command.
 */

/**
 * Provision install command
 *
 * These are the hooks that will be executed by the drush_invoke function
 * when doing a provision_install.
 */

/**
 * Check that we are trying to install a new site , and a new site only
 */
function drush_provision_drupal_provision_install_validate() {
  if (!drush_get_option('uri')) {
    return drush_set_error("PROVISION_URL_REQUIRED", dt("You need to specify a valid url to install a site"));
  }
  if (_provision_drupal_site_exists()) {
    return drush_set_error('PROVISION_SITE_INSTALLED');
  }
}

/**
 * Set up the directories and settings.php file that we need.
 */
function drush_provision_drupal_pre_provision_install() {
  // This is the actual drupal provisioning requirements. 
  _provision_drupal_create_directories();
}

/**
 * Install Drupal with the pre-configured settings, by calling an external script
 *
 * This is an external script so that php is running in it's own namespace, and
 * weird problems such as the multiple database connections don't confuse drupal.
 */
function drush_provision_drupal_provision_install() {
 // Requires at least the database settings to complete.
  provision_prepare_environment();
  _provision_drupal_create_settings_file();
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

  drush_include_engine('drupal', 'install');
  drush_set_option('installed', TRUE, 'site');
  _provision_drupal_maintain_aliases();
}

/**
 * If the install went south, and the site is not PROVISION_SITE_INSTALLED, clean up behind ourselves
 */
function drush_provision_drupal_provision_install_rollback() {
  if (drush_get_option('uri')) {
    if (!_provision_drupal_site_installed()) {
      _provision_recursive_delete('sites/' drush_get_option('uri'));
    }
  } else {
    drush_set_error('PROVISION_FRAMEWORK_ERROR', dt('no url defined in %function', array('%function' => __FUNCTION__)));
  }
}


/**
 * Finish the installation, regenerate the caches on the site so that
 * any changes to things such as available modules/ themes can take affect.
 */
function drush_provision_drupal_post_provision_install() {
  drush_set_option('aliases', drush_get_option('aliases'), 'site');
  drush_set_option('installed', TRUE, 'site');
  _provision_drupal_rebuild_caches();
  drush_set_option('packages', _scrub_object(provision_drupal_system_map()), 'site');
}


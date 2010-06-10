<?php

/**
 * Provision import command
 *
 * Import existing drupal sites
 */


/**
 * Import the information about the existing site, and return it in the $data context array
 */
function drush_provision_drupal_provision_import() {
  if (d()->type === 'site') {
    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
    drush_include_engine('drupal', 'import', drush_drupal_major_version());
    drush_set_option('installed', TRUE, 'site');
  }
}

function drush_provision_drupal_post_provision_import() {
  if (d()->type === 'site') {
    d()->profile = drush_get_option('profile', 'default', 'site');
    d()->language = drush_get_option('language', 'en', 'site');
    d()->write_alias();

    provision_backend_invoke(d()->name, 'provision-verify');
  }
}


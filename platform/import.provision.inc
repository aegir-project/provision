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
    d()->profile = drush_set_option('profile', drush_get_option('profile', 'default', 'site'));
    d()->language = drush_set_option('language', drush_get_option('language', 'en', 'site'));
    d()->aliases = drush_set_option('aliases', provision_drupal_find_aliases());
    d()->write_alias();

    // set this in the site options too so it recurrs.
    drush_set_option('aliases', drush_get_option('aliases'),  'site');

    // generate the drushrc
    provision_save_site_data();

    // Do not automatically save the drushrc at the end of the command.
    drush_set_option('provision_save_config', false);

    provision_backend_invoke(d()->name, 'provision-verify');
    
    drush_include_engine('drupal', 'cron_key');

    provision_reload_config('site');
  }
}


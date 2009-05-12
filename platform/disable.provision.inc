<?php

// $Id$

/**
 * Provision disable command
 *
 * Disable an enabled and installed site
 */

/**
 * Make sure the command we are running is on an installed site.
 */
function drush_provision_drupal_provision_disable_validate() {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

  if (!drush_get_option('installed')) {
     drush_set_error('PROVISION_DRUPAL_SITE_FOUND');
   }
}

/**
 * Make a backup of the site before we mess with it
 */
function drush_provision_drupal_pre_provision_disable($url, $backup_file = null) {
  drush_set_option('force', true, 'process');
  drush_invoke("provision backup", $url, $backup_file);
  drush_unset_option('force', 'process');
}

/**
 * Set the enabled property of the site to false
 */
function drush_provision_drupal_post_provision_disable() {
  drush_set_option('enabled', FALSE, 'site');
}


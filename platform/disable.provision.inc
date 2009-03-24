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
function provision_drupal_provision_disable_validate() {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

  if (!drush_get_option('installed')) {
     drush_set_error(PROVISION_DRUPAL_SITE_FOUND);
   }
}

/**
 * Make a backup of the site before we mess with it
 */
function provision_drupal_provision_pre_disable($url, $backup_file = null) {
  provision_invoke("backup", $url, $backup_file);
}

/**
 * Set the enabled property of the site to false
 */
function provision_drupal_provision_post_disable() {
  drush_set_option('enabled', FALSE, 'site');
}


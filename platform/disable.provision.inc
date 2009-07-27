<?php

// $Id$

/**
 * Provision disable command
 *
 * Disable an enabled and installed site
 */

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


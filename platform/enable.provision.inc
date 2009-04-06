<?php

/**
 * Provision enable command
 *
 * Enable a site that has been disabled
 */
function provision_drupal_provision_enable_validate() {
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_SITE);

}

function provision_drupal_post_provision_enable() {
  drush_set_option('enabled', TRUE, 'site');
}



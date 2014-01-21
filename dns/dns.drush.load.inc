<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is installed and enabled.
 */
function dns_drush_load() {
  $features = drush_get_option('hosting_features', array());
  return array_key_exists('dns', $features) && $features['dns'];
}

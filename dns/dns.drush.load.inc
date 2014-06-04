<?php

/**
 * Implements hook_drush_load(). Deprecated. Removed in Drush 7.x.
 *
 * Checks if the corresponsing Hosting Feature is installed and enabled.
 */
function dns_drush_load() {
  $features = drush_get_option('hosting_features', array());
  return array_key_exists('dns', $features) && $features['dns'];
}

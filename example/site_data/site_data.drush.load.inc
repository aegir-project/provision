<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is enabled.
 */
function site_data_drush_load() {
  $features = drush_get_option('hosting_features', array());
  return array_key_exists('site_data', $features) && $features['site_data'];
}

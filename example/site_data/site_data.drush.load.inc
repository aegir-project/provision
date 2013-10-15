<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is enabled.
 */
function site_data_drush_load() {
  return in_array('site_data', drush_get_option('hosting_features', array()));
}

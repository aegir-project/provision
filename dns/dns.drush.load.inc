<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is enabled.
 */
function dns_drush_load() {
  return in_array('dns', drush_get_option('hosting_features', array()));
}

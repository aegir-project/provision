<?php

/**
 * Implements hook_drush_load().
 *
 * Checks if the corresponsing Hosting Feature is enabled.
 */
function subdirs_drush_load() {
  return array_search('subdirs', drush_get_option('hosting_features', array()));
}

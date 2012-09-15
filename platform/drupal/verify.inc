<?php
/**
 *  @file
 *    Find modules, themes and themes engines
 */

if (drush_drupal_major_version() >= 7) {
  drush_set_option('modules', _scrub_object(system_rebuild_module_data()), 'site');
  drush_set_option('themes', _scrub_object(system_rebuild_theme_data()), 'site');
}
else {
  drush_set_option('modules', _scrub_object(module_rebuild_cache()), 'site');
  drush_set_option('themes', _scrub_object(system_theme_data()), 'site');
}

drush_set_option('engines', _scrub_object(drupal_system_listing('\.engine$', 'themes/engines')), 'site');

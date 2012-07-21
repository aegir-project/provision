<?php
// $Id$
/**
 *  @file
 *    Rebuild all the caches
 */

node_types_rebuild(); // d5, d6, d7, d8
drush_log(t('Rebuilt node types cache'));

drupal_flush_all_caches(); // d6, d7, d8
drush_log(t('All caches cleared'));

// Available in d6 and d7 only
//cache_clear_all();
//drush_log(t('Cleared all caches'));

// Included in drupal_flush_all_caches()
//registry_rebuild();
//drush_log(t('Rebuilt code registry'));

// Included in drupal_flush_all_caches()
//system_rebuild_theme_data();
//drush_log(t('Rebuilt theme cache'));

// Available in d5, d6, d7 only
//menu_rebuild();
//drush_log(t('Rebuilt menu cache'));

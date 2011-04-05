<?php
// $Id$
/**
 *  @file
 *    Rebuild all the caches
 */
cache_clear_all();
drush_log(t('Cleared all caches'));

node_types_rebuild();
drush_log(t('Rebuilt node type cache'));

module_rebuild_cache();
drush_log(t('Rebuilt module cache'));

drupal_rebuild_theme_registry();
system_theme_data();
drush_log(t('Rebuilt theme cache'));

menu_rebuild();
drush_log(t('Rebuilt menu cache'));

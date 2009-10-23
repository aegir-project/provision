<?php
// $Id$
/**
 *  @file
 *    Rebuild all the caches
 */
cache_clear_all();
drush_log(t('Cleared all caches'));

node_types_rebuild();
drush_log(t('Rebuild node type cache'));

module_rebuild_cache();
drush_log(t('Rebuild module cache'));

drupal_rebuild_theme_registry();
system_theme_data();
drush_log(t('Rebuild theme cache'));

node_access_rebuild();
drush_log(t('Rebuild node access cache'));

menu_rebuild();
drush_log(t('Rebuild menu cache'));

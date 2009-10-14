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

system_rebuild_module_data();
drush_log(t('Rebuild module cache'));

system_rebuild_theme_data();
drush_log(t('Rebuild theme cache'));

node_access_rebuild();
drush_log(t('Rebuild node access cache'));

menu_rebuild();
drush_log(t('Rebuild menu cache'));

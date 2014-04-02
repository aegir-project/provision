<?php
/**
 *  @file
 *    Rebuild all the caches
 */

cache_clear_all();
drush_log(dt('Cleared all caches'));

node_types_rebuild();
drush_log(dt('Rebuilt node type cache'));

module_list(TRUE, FALSE);
module_rebuild_cache();
drush_log(dt('Rebuilt module cache'));

system_theme_data();
drush_log(dt('Rebuilt theme cache'));

menu_rebuild();
drush_log(dt('Rebuilt menu cache'));

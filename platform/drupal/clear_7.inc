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

registry_rebuild();
drush_log(t('Rebuild code registry'));

system_rebuild_theme_data();
drush_log(t('Rebuild theme cache'));

menu_rebuild();
drush_log(t('Rebuild menu cache'));

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

registry_rebuild();
drush_log(t('Rebuilt code registry'));

system_rebuild_theme_data();
drush_log(t('Rebuilt theme cache'));

menu_rebuild();
drush_log(t('Rebuilt menu cache'));

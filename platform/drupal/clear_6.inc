<?php
/**
 *  @file
 *    Rebuild all the caches
 */

module_list(TRUE, FALSE);
module_rebuild_cache();
drush_log(dt('Rebuilt module cache'));
cache_clear_all('*', 'cache_form', TRUE);
drupal_flush_all_caches();
drush_log(dt('All caches flushed'));

<?php
/**
 *  @file
 *    Rebuild all the caches
 */

module_rebuild_cache();
drush_log(dt('Rebuilt module cache'));

drupal_flush_all_caches();
drush_log(dt('All caches flushed'));

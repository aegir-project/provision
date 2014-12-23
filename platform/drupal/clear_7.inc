<?php
/**
 *  @file
 *    Rebuild all the caches
 */

cache_clear_all('*', 'cache_form', TRUE);
drupal_flush_all_caches();
drush_log(dt('All caches flushed'));

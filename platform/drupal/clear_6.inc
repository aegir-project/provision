<?php
/**
 *  @file
 *    Rebuild all the caches
 */

drush_module_list();
module_rebuild_cache();
drush_log(dt('Rebuilt module cache'));

cache_clear_all('*', 'cache_form', TRUE); // This bin is excluded in drupal_flush_all_caches()
                                          // but if not purged, it may break tasks with
                                          // mysterious errors thanks to garbage collected.
drupal_flush_all_caches();
drush_log(dt('All caches flushed'));

// function drupal_flush_all_caches() {
//   // Change query-strings on css/js files to enforce reload for all users.
//   _drupal_flush_css_js();
//
//   drupal_clear_css_cache();
//   drupal_clear_js_cache();
//
//   // If invoked from update.php, we must not update the theme information in the
//   // database, or this will result in all themes being disabled.
//   if (defined('MAINTENANCE_MODE') && MAINTENANCE_MODE == 'update') {
//     _system_theme_data();
//   }
//   else {
//     system_theme_data();
//   }
//
//   drupal_rebuild_theme_registry();
//   menu_rebuild();
//   node_types_rebuild();
//   // Don't clear cache_form - in-progress form submissions may break.
//   // Ordered so clearing the page cache will always be the last action.
//   $core = array('cache', 'cache_block', 'cache_filter', 'cache_page');
//   $cache_tables = array_merge(module_invoke_all('flush_caches'), $core);
//   foreach ($cache_tables as $table) {
//     cache_clear_all('*', $table, TRUE);
//   }
// }

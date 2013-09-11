<?php
/**
 *  @file
 *    Rebuild all the caches
 */

drupal_flush_all_caches();
drush_log(t('All caches flushed'));

// function drupal_flush_all_caches() {
//   // Change query-strings on css/js files to enforce reload for all users.
//   _drupal_flush_css_js();
//
//   registry_rebuild();
//   drupal_clear_css_cache();
//   drupal_clear_js_cache();
//
//   // Rebuild the theme data. Note that the module data is rebuilt above, as
//   // part of registry_rebuild().
//   system_rebuild_theme_data();
//   drupal_theme_rebuild();
//
//   entity_info_cache_clear();
//   node_types_rebuild();
//   // node_menu() defines menu items based on node types so it needs to come
//   // after node types are rebuilt.
//   menu_rebuild();
//
//   // Synchronize to catch any actions that were added or removed.
//   actions_synchronize();
//
//   // Don't clear cache_form - in-progress form submissions may break.
//   // Ordered so clearing the page cache will always be the last action.
//   $core = array(
//     'cache',
//     'cache_path',
//     'cache_filter',
//     'cache_bootstrap',
//     'cache_page',
//   );
//   $cache_tables = array_merge(module_invoke_all('flush_caches'), $core);
//   foreach ($cache_tables as $table) {
//     cache_clear_all('*', $table, TRUE);
//   }
//
//   // Rebuild the bootstrap module list. We do this here so that developers
//   // can get new hook_boot() implementations registered without having to
//   // write a hook_update_N() function.
//   _system_update_bootstrap_status();
// }

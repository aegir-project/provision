<?php
/**
 *  @file
 *    Rebuild all the caches
 */

drupal_flush_all_caches();
drush_log(t('All caches flushed'));

// function drupal_flush_all_caches() {
//   // Flush all persistent caches.
//   // This is executed based on old/previously known information, which is
//   // sufficient, since new extensions cannot have any primed caches yet.
//   foreach (module_invoke_all('cache_flush') as $bin) {
//     cache($bin)->flush();
//   }
//
//   // Flush asset file caches.
//   drupal_clear_css_cache();
//   drupal_clear_js_cache();
//   _drupal_flush_css_js();
//
//   // Reset all static caches.
//   drupal_static_reset();
//
//   // Clear all non-drupal_static() static caches.
//   // None currently; kept if any static caches need to be reset in the future.
//
//   // Rebuild module and theme data.
//   system_rebuild_module_data();
//   system_rebuild_theme_data();
//
//   // Ensure that all modules that are currently supposed to be enabled are
//   // actually loaded.
//   module_load_all();
//
//   // Update the list of bootstrap modules.
//   // Allows developers to get new hook_boot() implementations registered without
//   // having to write a hook_update_N() function.
//   _system_update_bootstrap_status();
//
//   // Rebuild the schema and cache a fully-built schema based on new module data.
//   // This is necessary for any invocation of index.php, because setting cache
//   // table entries requires schema information and that occurs during bootstrap
//   // before any modules are loaded, so if there is no cached schema,
//   // drupal_get_schema() will try to generate one, but with no loaded modules,
//   // it will return nothing.
//   drupal_get_schema(NULL, TRUE);
//
//   // Rebuild all information based on new module data.
//   module_invoke_all('rebuild');
//
//   // Rebuild the menu router based on all rebuilt data.
//   // Important: This rebuild must happen last, so the menu router is guaranteed
//   // to be based on up to date information.
//   menu_router_rebuild();
//
//   // Re-initialize the maintenance theme, if the current request attempted to
//   // use it. Unlike regular usages of this function, the installer and update
//   // scripts need to flush all caches during GET requests/page building.
//   if (function_exists('_drupal_maintenance_theme')) {
//     unset($GLOBALS['theme']);
//     drupal_maintenance_theme();
//   }
// }

// function drupal_flush_all_caches() {
//   // Flush all persistent caches.
//   // This is executed based on old/previously known information, which is
//   // sufficient, since new extensions cannot have any primed caches yet.
//   foreach (module_invoke_all('cache_flush') as $bin) {
//     cache($bin)->flush();
//   }
//
//   // Flush asset file caches.
//   drupal_clear_css_cache();
//   drupal_clear_js_cache();
//   _drupal_flush_css_js();
//
//   // Reset all static caches.
//   drupal_static_reset();
//
//   // Clear all non-drupal_static() static caches.
//   // None currently; kept if any static caches need to be reset in the future.
//
//   // Update and synchronize the class registry and extension information based
//   // on current/actual code.
//   // Module data is rebuilt as part of registry_rebuild().
//   registry_rebuild();
//   system_rebuild_theme_data();
//
//   // Ensure that all modules that are currently supposed to be enabled are
//   // actually loaded.
//   module_load_all();
//
//   // Update the list of bootstrap modules.
//   // Allows developers to get new hook_boot() implementations registered without
//   // having to write a hook_update_N() function.
//   _system_update_bootstrap_status();
//
//   // Rebuild the schema and cache a fully-built schema based on new module data.
//   // This is necessary for any invocation of index.php, because setting cache
//   // table entries requires schema information and that occurs during bootstrap
//   // before any modules are loaded, so if there is no cached schema,
//   // drupal_get_schema() will try to generate one, but with no loaded modules,
//   // it will return nothing.
//   drupal_get_schema(NULL, TRUE);
//
//   // Rebuild all information based on new module data.
//   module_invoke_all('rebuild');
//
//   // Rebuild the menu router based on all rebuilt data.
//   // Important: This rebuild must happen last, so the menu router is guaranteed
//   // to be based on up to date information.
//   menu_router_rebuild();
//
//   // Re-initialize the maintenance theme, if the current request attempted to
//   // use it. Unlike regular usages of this function, the installer and update
//   // scripts need to flush all caches during GET requests/page building.
//   if (function_exists('_drupal_maintenance_theme')) {
//     unset($GLOBALS['theme']);
//     drupal_maintenance_theme();
//   }
// }

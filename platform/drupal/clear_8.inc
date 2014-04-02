<?php
/**
 *  @file
 *    Rebuild all the caches
 */

drupal_flush_all_caches();
drush_log(dt('All caches flushed'));

// function drupal_flush_all_caches() {
//   $module_handler = \Drupal::moduleHandler();
//   // Flush all persistent caches.
//   // This is executed based on old/previously known information, which is
//   // sufficient, since new extensions cannot have any primed caches yet.
//   $module_handler->invokeAll('cache_flush');
//   foreach (Cache::getBins() as $service_id => $cache_backend) {
//     if ($service_id != 'cache.menu') {
//       $cache_backend->deleteAll();
//     }
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
//   \Drupal::entityManager()->clearCachedDefinitions();
//
//   // Wipe the PHP Storage caches.
//   PhpStorageFactory::get('service_container')->deleteAll();
//   PhpStorageFactory::get('twig')->deleteAll();
//
//   // Rebuild module and theme data.
//   $module_data = system_rebuild_module_data();
//   system_rebuild_theme_data();
//
//   // Rebuild and reboot a new kernel. A simple DrupalKernel reboot is not
//   // sufficient, since the list of enabled modules might have been adjusted
//   // above due to changed code.
//   $files = array();
//   foreach ($module_data as $module => $data) {
//     if (isset($data->uri) && $data->status) {
//       $files[$module] = $data->uri;
//     }
//   }
//   \Drupal::service('kernel')->updateModules($module_handler->getModuleList(), $files);
//   // New container, new module handler.
//   $module_handler = \Drupal::moduleHandler();
//
//   // Ensure that all modules that are currently supposed to be enabled are
//   // actually loaded.
//   $module_handler->loadAll();
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
//   $module_handler->invokeAll('rebuild');
//
//   // Rebuild the menu router based on all rebuilt data.
//   // Important: This rebuild must happen last, so the menu router is guaranteed
//   // to be based on up to date information.
//   \Drupal::service('router.builder')->rebuild();
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

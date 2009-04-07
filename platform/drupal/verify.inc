<?php
// $Id$

drush_set_option('modules', _scrub_object(module_rebuild_cache()), 'site');
// Find theme engines
drush_set_option('engines', _scrub_object(drupal_system_listing('\.engine$', 'themes/engines')), 'site');
drush_set_option('themes', _scrub_object(system_theme_data()), 'site');


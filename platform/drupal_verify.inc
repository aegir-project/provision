<?php
// $Id$

$data['modules'] = module_rebuild_cache();
// Find theme engines
$data['engines'] = drupal_system_listing('\.engine$', 'themes/engines');
$data['themes'] = system_theme_data();


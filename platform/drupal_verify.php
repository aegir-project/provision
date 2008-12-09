<?php
// $Id$

require_once(dirname(__FILE__) . '/../provision.inc');
require_once('provision_drupal.module');

$url = ($argv[1]) ? $argv[1] : null;
provision_external_init($url);

$data['modules'] = module_rebuild_cache();
// Find theme engines
$data['engines'] = drupal_system_listing('\.engine$', 'themes/engines');
$data['themes'] = system_theme_data();

provision_output($data);

<?php
// $Id$
/**
 *  @file
 *    Rebuild all the caches
 */

if ($argv[1]) {
  // Fake the necessary HTTP headers that Drupal needs:
  $drupal_base_url = parse_url(sprintf("http://" . $argv[1]));
  $_SERVER['HTTP_HOST'] = $drupal_base_url['host'];
  $_SERVER['PHP_SELF'] = $drupal_base_url['path'].'/index.php';
  $_SERVER['REQUEST_URI'] = $_SERVER['SCRIPT_NAME'] = $_SERVER['PHP_SELF'];
  $_SERVER['REMOTE_ADDR'] = '';
  $_SERVER['REQUEST_METHOD'] = NULL;
  $_SERVER['SERVER_SOFTWARE'] = NULL;

  require_once('includes/bootstrap.inc');
  drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
  require_once(dirname(__FILE__) . '/../provision.inc');
}
else {
  print "USAGE: provision_drupal_clear.php url\n";
  exit(PROVISION_FRAMEWORK_ERROR);
}

cache_clear_all();
provision_log('notice', t('Cleared all caches'));

node_types_rebuild();
provision_log('notice', t('Rebuild node type cache'));

module_rebuild_cache();
provision_log('notice', t('Rebuild module cache'));

system_theme_data();
provision_log('notice', t('Rebuild theme cache'));

node_access_rebuild();
provision_log('notice', t('Rebuild node access cache'));

menu_rebuild();
provision_log('notice', t('Rebuild menu cache'));

provision_output($argv[1], array());


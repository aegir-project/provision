<?php

/**
 * Implementation of hook_provision_pre_restore
 */
function drush_http_pre_provision_restore($url) {
  drush_set_option('redirect_url', drush_get_option('web_maintenance_url'));
  provision_service('http')->create_site_config($url);
  provision_service('http')->parse_configs();
}

function drush_http_pre_provision_restore_rollback($url) {
  provision_service('http')->create_site_config($url);
  provision_service('http')->parse_configs();
}

function drush_http_post_provision_restore($url) {
  provision_service('http')->create_site_config($url);
  provision_service('http')->parse_configs();

}

// The old db details should have been switched back by the mysql rollback at this point.
function drush_http_post_provision_restore_rollback($url) {
  drush_http_post_provision_restore($url);
}

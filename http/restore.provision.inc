<?php

/**
 * Implementation of hook_provision_pre_restore
 */
function drush_http_pre_provision_restore() {
  drush_set_option('redirect_url', drush_get_option('web_maintenance_url'));
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();
}

function drush_http_pre_provision_restore_rollback() {
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();
}

function drush_http_post_provision_restore() {
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();

}

// The old db details should have been switched back by the mysql rollback at this point.
function drush_http_post_provision_restore_rollback() {
  drush_http_post_provision_restore();
}

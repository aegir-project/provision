<?php

function drush_http_post_provision_deploy() {
  provision_service('http')->create_site_config();
  provision_service('http')->parse_configs();
}

/**
 * Delete the config file created.
 */
function drush_http_post_provision_deploy_rollback() {
  provision_service('http')->create_site_config();
  provision_service('http')->parse_configs();
}

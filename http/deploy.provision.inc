<?php

function drush_http_post_provision_deploy($url = NULL) {
  provision_service('http')->create_site_config($url);
  provision_service('http')->parse_configs();
}

/**
 * Delete the config file created.
 */
function drush_http_post_provision_deploy_rollback($url = NULL) {
  provision_service('http')->create_site_config($url);
  provision_service('http')->parse_configs();
}

<?php

function drush_http_post_provision_deploy() {
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();
}

/**
 * Delete the config file created.
 */
function drush_http_post_provision_deploy_rollback() {
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();
}



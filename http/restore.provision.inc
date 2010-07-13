<?php

/**
 * Implementation of hook_provision_pre_restore
 */
function drush_http_pre_provision_restore() {
  d()->service('http')->delete_config('site'); 
}

function drush_http_pre_provision_restore_rollback() {
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();
}


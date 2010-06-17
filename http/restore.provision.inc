<?php

/**
 * Implementation of hook_provision_pre_restore
 */
function drush_http_pre_provision_restore() {
  d()->service('http')->delete_site_config(); 
}

function drush_http_pre_provision_restore_rollback() {
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();
}


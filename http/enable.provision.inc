<?php

/**
 * Implementation of hook_provision_enable
 */
function drush_http_provision_enable() {
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();
}


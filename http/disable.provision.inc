<?php

/**
 * Implementation of hook_provision_disable
 */
function drush_http_provision_disable() {
  drush_set_option('redirect_url', drush_get_option('web_disable_url'));

  // TODO - this is majorly wrong i think.
  d()->service('http')->create_config('site');
  d()->service('http')->parse_configs();
}


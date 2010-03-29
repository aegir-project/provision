<?php

/**
 * Implementation of hook_provision_enable
 */
function drush_http_provision_enable($url = NULL) {
  provision_service('http')->create_site_config($url);
  provision_service('http')->parse_configs();
}


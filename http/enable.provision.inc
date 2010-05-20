<?php

/**
 * Implementation of hook_provision_enable
 */
function drush_http_provision_enable() {
  provision_service('http')->create_site_config();
  provision_service('http')->parse_configs();
}


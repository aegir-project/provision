<?php

function drush_http_provision_delete() {
  if (d()->type === 'site') {
    d()->service('http')->delete_site_config();
  }
  if (d()->type === 'platform') {
    $sites = provision_drupal_find_sites();
    if($sites) {
      drush_set_error(dt('Existing sites were found on this platform. These sites will need to be deleted before this platform can be deleted.'));
    }
    else {
      d()->service('http')->delete_platform_config();

    }
  }
  return d()->service('http')->parse_configs();
}


<?php

function drush_http_provision_delete($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    provision_service('http')->delete_site_config($url);
  }
  if (PROVISION_CONTEXT_PLATFORM) {
    $sites = provision_drupal_find_sites();
    if($sites) {
      drush_set_error(dt('Existing sites were found on this platform. These sites will need to be deleted before this platform can be deleted.'));
    }
    else {
      provision_service('http')->delete_platform_config();

    }
  }
  return provision_service('http')->parse_configs();
}


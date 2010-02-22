<?php

function drush_provision_apache_provision_delete($url = NULL) {
  if (PROVISION_CONTEXT_SITE) {
    _provision_apache_delete_vhost_config($url);
  }
  if (PROVISION_CONTEXT_PLATFORM) {
    $options = drush_get_merged_options();
    $sites = provision_drupal_find_sites();
    if($sites) {
      drush_set_error(dt('Existing sites were found on this platform. These sites will need to be deleted before this platform can be deleted.'));
    }
    else {
      _provision_apache_delete_platform_config('platform_' . $options['platform'] . '.conf');
    }
  }
  return _provision_apache_restart_apache();  
}


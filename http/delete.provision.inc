<?php

function drush_http_provision_delete() {
  if (d()->type === 'site') {
    d()->service('http')->delete_config('site');
  }
  if (d()->type === 'platform') {
    if (!drush_get_option('force', FALSE) && drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_ROOT) && provision_drupal_find_sites()) {
      drush_set_error(dt('Existing sites were found on this platform. These sites will need to be deleted before this platform can be deleted.'));
    }
    else {
      d()->service('http')->delete_config('platform');
    }
  }
  d()->service('http')->parse_configs();
}

<?php

/**
 * @file manual site migrate system
 */

function drush_provision_hostmaster_migrate_validate($site, $platform) {
  // verify the target platform:
  $data = drush_get_merged_options(); // XXX: necessary because the remote
                                      // platform may not have been
                                      // initialised by the frontend (this
                                      // should be checked instead)
  $data['root'] = $platform;
  $data['r'] = $platform;
  drush_backend_invoke('provision verify', $data);
  // verify the current platform
  drush_backend_invoke('provision verify', array('root' => drush_get_option(array('r', 'root'), drush_locate_root())));
  // verify the site prior to migration
  drush_backend_invoke('provision verify', array($site));
}

function drush_provision_pre_hostmaster_migrate($site, $platform) {
  if (!drush_confirm("This command will replace your crontab entry. continue?")) {
    drush_set_error('PROVISION_CANCEL_MIGRATE', 'Cancelling');
  }
  // wipe out cron entry
  exec('crontab -r');

  // we can't rely on the park command to exist in pre-0.4 alpha 2 releases
  drush_backend_invoke('hostmaster park', array($site, 'uri' => $site));
}

function drush_provision_hostmaster_migrate($site, $platform) { 
  drush_backend_invoke('provision clone', array($site, $site, $platform));
}

function drush_provision_hostmaster_migrate_rollback($site, $platform) {
  drush_backend_invoke('provision delete ', array($site, 'root' => $platform));
}

function drush_provision_post_hostmaster_migrate($site, $platform) {
  drush_backend_invoke('hostmaster unpark', array($site, 'uri' => $site, 'root' => $platform));
  if (!drush_get_error()) {
    drush_backend_invoke('provision delete', array($site));
    drush_backend_invoke('provision verify', array($site, 'root' => $platform, 'publish_path' => $platform));
  }
}

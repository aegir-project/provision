<?php

/**
 * @file manual site migrate system
 */

function drush_provision_hostmaster_migrate_validate($site, $platform) {
  // verify the target platform:
  if (!is_dir($platform)) {
    drush_print("The platform path you specified does not exist yet.");
    drush_print("Downloading the hostmaster front end. (this might take a few minutes)");
    drush_backend_invoke('make', array(dirname(__FILE__) . '/aegir.make', $platform));
    if (drush_get_error()) {
      drush_log("The hostmaster front end could not be downloaded, exiting.", "error");
    }
  }

  $data = drush_get_merged_options(); 
  $data['r'] = $data['root'] = realpath($platform);
  drush_backend_invoke('provision-verify', $data);
  // verify the current platform
  drush_backend_invoke('provision-verify', array('root' => drush_get_option(array('r', 'root'), drush_locate_root())));
  // verify the site prior to migration
  drush_backend_invoke('provision-verify', array($site));
}

function drush_provision_pre_hostmaster_migrate($site, $platform) {
  if (!drush_confirm("This command will replace your crontab entry. continue?")) {
    return drush_set_error('PROVISION_CANCEL_MIGRATE', 'Cancelling');
  }
  // wipe out cron entry
  exec('crontab -r');

  // we can't rely on the park command to exist in pre-0.4 alpha 2 releases
  drush_backend_invoke('hostmaster-pause', array($site, 'uri' => $site));
}

function drush_provision_hostmaster_migrate($site, $platform) { 
  drush_backend_invoke('provision-migrate', array($site, $platform));
}

function drush_provision_post_hostmaster_migrate($site, $platform) {
  drush_backend_invoke('hostmaster-resume', array($site, 'uri' => $site, 'root' => $platform));
}


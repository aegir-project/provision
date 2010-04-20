<?php

/**
 * @file install the hostmaster system
 */

function drush_provision_hostmaster_install($site, $client_email) {
  $platform = drush_locate_root(drush_get_option(array('r', 'root'), NULL));
  // verify the current platform
  drush_backend_invoke('provision-verify', array(
    'root' => $platform,
    'provision_context' => "platform",
    'publish_path' => $platform,
    'platform' => '5') // dirty hardcoded to create the first platform apache config
                       // this will become unnecessary when we have drush aliases
  );

  // install the hostmaster site
  drush_backend_invoke('provision-install', array(
    $site, 
    'uri' => $site, 
    'root' => $platform, 
    'profile' => "hostmaster",
    'provision_context' => "site",
    'client_email' => $client_email,
    'site_port' => drush_get_option('web_port', 80))
  );

}


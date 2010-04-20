<?php

/**
 * @file manual site migrate system
 */

function drush_provision_hostmaster_install($site, $platform, $client_email) {
  $data = drush_get_merged_options();
  $data['publish_path'] = $data['r'] = $data['root'] = realpath($platform);
  $data['provision_context'] = "server";
  $data['init_http'] = "apache";
  $data['restart_cmd'] = _provision_default_restart_cmd();
  $data['web_port'] = drush_get_option('web_port', 80);

  // Server verify
  drush_backend_invoke('provision-verify', $data);

  // verify the current platform
  drush_backend_invoke('provision-verify', array(
    'root' => $platform, //drush_get_option(array('r', 'root'), $platform),
    'provision_context' => "platform",
    'publish_path' => drush_locate_root(drush_get_option(array('r', 'root'), NULL)),
    'platform' => '5') // dirty hardcoded to create the first platform apache config
                       // this will become unnecessary when we have drush 
  );

  // verify the site prior to migration
  drush_backend_invoke('provision-install', array(
    $site, 
    'uri' => $site, 
    'root' => $platform, 
    'profile' => "hostmaster",
    'provision_context' => "site",
    'client_email' => $client_email,
    'port' => drush_get_option('web_port', 80))
  );

}


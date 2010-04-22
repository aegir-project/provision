<?php

/**
 * @file install the hostmaster system
 */

function drush_provision_hostmaster_install($site) {
  $version = drush_get_option('version', 'HEAD');
  $platform = drush_get_option(array('r', 'root'), drush_server_home() . '/' . 'hostmaster-' . $version);

  if (!drush_get_option('backend-only')) {
    drush_backend_invoke('hostmaster-make', array($platform));
  }

  $data['master_db'] = drush_get_option('master_db');

  if (!$data['master_db']) {
    drush_print(dt("No database configuration provided, asking for one interactively"));
    $scheme = drush_prompt(dt("Database type"), "mysql");
    $host = drush_prompt(dt("Database server"), "localhost");
    $user = drush_prompt(dt("Database user"), "root");
    system('stty -echo');
    $pass = drush_prompt(dt("Database password"));
    system('stty echo');
    print "\n"; # add a cr since the user's didn't echo
    $data['master_db'] = "$scheme://$user:$pass@$host";
  }
  
  $data['init_http'] = 'apache';
  $data['init_db'] = 'mysql';
  $data['provision_context'] = 'server';
  $data['parent_path'] = drush_server_home();
  $data['script_user'] = drush_get_option('script_user');
  $data['web_group'] = drush_get_option('web_group', _provision_default_web_group());
  $data['web_port'] = drush_get_option('web_port', 80);
  $data['drush_path'] = DRUSH_COMMAND;

  // verify the server
  drush_backend_invoke('provision-verify', $data, 'POST');

  if (drush_get_option('backend-only')) {
    return;
  }

  // verify the current platform
  drush_backend_invoke('provision-verify', array(
    'root' => $platform,
    'provision_context' => "platform",
    'publish_path' => $platform,
    'platform' => '5') // dirty hardcoded to create the first platform apache config
                       // this will become unnecessary when we have drush aliases
  );

  // install the hostmaster site
  $data = drush_backend_invoke('provision-install', array(
    $site, 
    'uri' => $site, 
    'root' => $platform, 
    'client_email' => drush_get_option('client_email', 'webmaster@localhost'),
    'profile' => "hostmaster",
    'provision_context' => "site",
    'site_port' => drush_get_option('web_port', 80)),
    'GET', TRUE
  );

  drush_print("Initializing the hosting system");
  drush_backend_invoke('hosting-setup', array('uri' => $site, 'root' => $platform));

  drush_print(dt("Aegir is now installed. You can visit it at @link", array('@link' => $data['context']['login_link'])));
}


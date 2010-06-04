<?php

/**
 * @file install the hostmaster system
 */

function drush_provision_hostmaster_install($site) {
  if (!$site) {
    return drush_set_error('SITE_MISSING', dt('You need to specify the FQDN of the hostmaster install'));
  }

  $version = drush_get_option('version', 'HEAD');
  $aegir_root = drush_get_option('aegir_root', drush_server_home());
  $platform = drush_get_option(array('r', 'root'), $aegir_root . '/' . 'hostmaster-' . $version);

  if (!drush_get_option('backend-only')) {

    if (!function_exists('drush_make_drush_command')) {
      drush_backend_invoke('dl', array('drush_make-' . drush_get_option('drush_make_version', '6.x-2.0-beta6'), 'destination' => $aegir_root . '/.drush/'));
    }

    drush_print(dt("deploying hostmaster application"));
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

  $data['hosting_name'] = 'server_localhost';
  $data['provision-service-http'] = 'apache';
  $data['provision-service-db'] = 'mysql';
  $data['provision-context'] = 'server';
  $data['aegir_root'] = $aegir_root;
  $data['script_user'] = drush_get_option('script_user', 'aegir');
  $data['web_group'] = drush_get_option('web_group', _provision_default_web_group());
  $data['web_port'] = drush_get_option('web_port', 80);
  $data['config_path'] = drush_get_option('config_path', $aegir_root . '/config');
  $data['backup_path'] = drush_get_option('backup_path', $aegir_root . '/backups');
  $data['drush_path'] = DRUSH_COMMAND;

  // verify the server
  drush_backend_invoke('provision-named-context-save', $data, 'POST');
  drush_do_site_command(drush_sitealias_get_record('@server_localhost'), 'provision-verify');

  if (drush_get_option('backend-only')) {
    return;
  }

  // verify the current platform
  drush_backend_invoke('provision-named-context-save', array(
    'parent' => '@server_localhost-apache', // todo should match future node title
    'hosting_name' => 'platform_Hostmaster', // todo should match future node title
    'provision-context' => 'platform',
    'root' => $platform,
  ));
  drush_do_site_command(drush_sitealias_get_record('@platform_Hostmaster'), 'provision-verify');

  // install the hostmaster site
  $data = drush_backend_invoke('provision-install', array(
    'hosting_name' => '',//todo
    'parent' => '@server_localhost-mysql,@platform_Hostmaster',
    'uri' => $site, 
    'client_email' => drush_get_option('client_email', 'webmaster@localhost'),
    'profile' => "hostmaster",
    'provision-context' => "site",
    'site_port' => drush_get_option('web_port', 80)),
    'GET', TRUE
  );

  drush_print("Initializing the hosting system");
  drush_backend_invoke('hosting-setup', array('uri' => $site, 'root' => $platform));

  drush_print(dt("Aegir is now installed. You can visit it at @link", array('@link' => $data['context']['login_link'])));
}


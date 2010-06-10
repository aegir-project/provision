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

  $server = '@server_localhost';
  drush_backend_invoke_args("provision-save", array($server), array(
    'context_type' => 'server',
   // files
    'file_service_type' => 'local',
    'aegir_root' => $aegir_root,
    'config_path' => $aegir_root . '/config',
    'backup_path' => $aegir_root . '/backups',
    'script_user' => drush_get_option('script_user', 'aegir'),
   // apache 
    'http_service_type' => 'apache',
    'web_group' => drush_get_option('web_group', _provision_default_web_group()),
   // mysql
    'db_service_type' => 'mysql',
    'master_db' => $data['master_db'],

  ));
  provision_backend_invoke($server, 'provision-verify');

  if (drush_get_option('backend-only')) {
    return;
  }

  $platform_name = '@platform_hostmaster';
  drush_backend_invoke_args("provision-save", array($platform_name), array(
    'context_type' => 'platform',
    'server' => $server,
    'web_server' => $server,
    'root' => $platform,
  ));
  provision_backend_invoke($platform_name, 'provision-verify');


  $site_name = '@hostmaster';
  drush_backend_invoke_args("provision-save", array($site_name), array(
    'context_type' => 'site',
    'platform' => $platform_name,
    'db_server' => $server,
    'uri' => $site,
    'client_email' => drush_get_option('client_email', 'webmaster@example.com'),
    'site_port' => drush_get_option('web_port', 80),
    'profile' => 'hostmaster',
  ));
  $data = provision_backend_invoke($site_name, 'provision-install');
  provision_backend_invoke($site_name, 'provision-verify');

  drush_print("Initializing the hosting system");
  provision_backend_invoke($site_name, 'hosting-setup');

  drush_print(dt("Aegir is now installed. You can visit it at @link", array('@link' => $data['context']['login_link'])));
}


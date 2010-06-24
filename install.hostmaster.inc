<?php

/**
 * @file install the hostmaster system
 */

function drush_provision_hostmaster_install($site = NULL) {
  if (!$site) {
    $site = drush_prompt(dt("Aegir domain name"), "aegir.example.com");
  }

  $version = drush_get_option('version', 'HEAD');
  $aegir_root = drush_get_option('aegir_root', drush_server_home());
  $platform = drush_get_option(array('r', 'root'), $aegir_root . '/' . 'hostmaster-' . $version);

  if (!drush_get_option('backend-only')) {

    if (!function_exists('drush_make_drush_command')) {
      drush_backend_invoke('dl', array('drush_make-' . drush_get_option('drush_make_version', '6.x-2.0-beta6'), 'destination' => $aegir_root . '/.drush/'));
    }

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

  // TODO: support creation of an external db server
  $server = '@server_master';
  drush_backend_invoke_args("provision-save", array($server), array(
    'context_type' => 'server',
    // files
    'remote_host' => php_uname('n'),
    'aegir_root' => $aegir_root,
    'script_user' => drush_get_option('script_user', get_current_user()),
   // apache 
    'http_service_type' => 'apache',
    'web_group' => drush_get_option('web_group', _provision_default_web_group()),
    'master_url' => "http://" . $site . "/",
   // mysql
    'db_service_type' => 'mysql',
    'master_db' => $data['master_db'],
  ));
  provision_backend_invoke($server, 'provision-verify');

  // exit if an error has occured. 
  if (drush_get_error()) {
    return false;
  }

  if (drush_get_option('backend-only')) {
    return;
  }

  $platform_name = '@platform_hostmaster';
  drush_backend_invoke_args("provision-save", array($platform_name), array(
    'context_type' => 'platform',
    'server' => $server,
    'web_server' => $server,
    'root' => $platform,
    'makefile' => $aegir_root . '/.drush/provision/aegir.make',
  ));
  provision_backend_invoke($platform_name, 'provision-verify');

  // exit if an error has occured. 
  if (drush_get_error()) {
    return false;
  }


  if (!drush_get_option('client_email')) {
    $client_email = drush_prompt(dt("Admin user e-mail"), "webmaster@localhost");
  }
  else {
    $client_email = drush_get_option('client_email');
  }

  $site_name = '@hostmaster';
  drush_backend_invoke_args("provision-save", array($site_name), array(
    'context_type' => 'site',
    'platform' => $platform_name,
    'db_server' => $server,
    'uri' => $site,
    'client_email' => $client_email,
    'profile' => 'hostmaster',
  ));
  $data = provision_backend_invoke($site_name, 'provision-install');
  provision_backend_invoke($site_name, 'provision-verify');

  // exit if an error has occured. 
  if (drush_get_error()) {
    return false;
  }


  drush_print("Initializing the hosting system");
  provision_backend_invoke($site_name, 'hosting-setup');

  drush_print(dt("Aegir is now installed. You can visit it at @link", array('@link' => $data['context']['login_link'])));
}


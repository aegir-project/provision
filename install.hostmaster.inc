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
      drush_backend_invoke('dl', array('drush_make-' . drush_get_option('drush_make_version', '6.x-2.0-beta9'), 'destination' => $aegir_root . '/.drush/'));
    }

  }

  $aegir_host = drush_get_option('aegir_host', php_uname('n'), 'options');
  $aegir_db_pass = drush_get_option('aegir_db_pass', 'pass', 'options');
  $aegir_db_user = drush_get_option('aegir_db_user', 'aegir_root', 'options');
  $master_db = sprintf("mysql://%s:%s@%s",$aegir_db_user, $aegir_db_pass, $aegir_host);

  // TODO: support creation of an external db server
  $server = '@server_master';
  drush_backend_invoke_args("provision-save", array($server), array(
    'context_type' => 'server',
    // files
    'remote_host' => $aegir_host,
    'aegir_root' => $aegir_root,
    'script_user' => drush_get_option('script_user', provision_current_user()),
   // apache or nginx or..
    'http_service_type' => drush_get_option('http_service_type', 'apache', 'options'),
    'web_group' => drush_get_option('web_group', _provision_default_web_group()),
    'master_url' => "http://" . $site,
   // mysql
    'db_service_type' => 'mysql',
    'master_db' => $master_db,
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


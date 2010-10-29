<?php

/**
 * @file install the hostmaster system
 */

function drush_provision_hostmaster_install_validate($site = NULL) {
  $version = drush_get_option('version', 'HEAD');
  drush_print("Aegir $version automated install script");
  drush_print("==============================================================================");

  drush_print('
This script will operate the following changes in your system:

1. Create server-level configuration directories
2. Download drush_make
3. Create the Hostmaster frontend platform
4. Install the frontend site
5. Setup the dispatcher (a user cron job)

We are making the following assumptions:
 * you have read INSTALL.txt and prepared the platform accordingly
 * you are executing this script as your "aegir" user
');

  $go = FALSE;
  while (!$go) {
    $yesno = trim(strtolower(drush_prompt(dt('Do you really want to proceed with the install'), 'Y/n', TRUE)));

    switch ($yesno) {
    case 'no':
    case 'n':
      return drush_set_error(dt('Installation aborted by user'));
      break;
    case 'yes':
    case 'y':
    case 'y/n':
      $go = TRUE;
      break;
    default:
      drush_print(dt('Please answer "yes" or "no"'));
      break;
    }
  }
  return TRUE;
}

function drush_provision_hostmaster_install($site = NULL) {
  if (!$site) {
    $site = drush_prompt(dt("Aegir domain name"), "aegir.example.com");
  }

  $version = drush_get_option('version', 'HEAD');
  $aegir_root = drush_get_option('aegir_root', drush_server_home());
  $platform = drush_get_option(array('r', 'root'), $aegir_root . '/' . 'hostmaster-' . $version);

  $aegir_host = drush_get_option('aegir_host', php_uname('n'), 'options');
  $aegir_db_user = drush_get_option('aegir_db_user', 'root', 'options');
  $aegir_db_pass = drush_get_option('aegir_db_pass', NULL, 'options');
  if (!$aegir_db_pass) {
    # XXX: may not be portable everywhere?
    system('stty -echo');
    $aegir_db_pass = drush_prompt(dt('MySQL privileged user ("!root") password', array('!root' => $aegir_db_user)));
    system('stty echo');
    print "\n"; # add a newline since the user's didn't print
  }
  
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

  if (!function_exists('drush_make_drush_command')) {
    drush_backend_invoke('dl', array('drush_make-' . drush_get_option('drush_make_version', '6.x-2.0-beta9'), 'destination' => $aegir_root . '/.drush/'));
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

  while (!filter_var(drush_get_option('client_email'), FILTER_VALIDATE_EMAIL)) {
   $client_email = drush_prompt(dt("Admin user e-mail"), "you@example.com");
   drush_set_option('client_email', $client_email);
  }

  $site_name = '@hostmaster';
  drush_backend_invoke_args("provision-save", array($site_name), array(
    'context_type' => 'site',
    'platform' => $platform_name,
    'db_server' => $server,
    'uri' => $site,
    'client_email' => drush_get_option('client_email'),
    'profile' => 'hostmaster',
  ));
  $data = provision_backend_invoke($site_name, 'provision-install');
  provision_backend_invoke($site_name, 'provision-verify');

  // exit if an error has occured. 
  if (drush_get_error()) {
    return false;
  }


  drush_print(dt("Initializing the hosting system"));
  provision_backend_invoke($site_name, 'hosting-setup');

  drush_print(dt("Aegir is now installed. You can visit it at @link", array('@link' => $data['context']['login_link'])));
}


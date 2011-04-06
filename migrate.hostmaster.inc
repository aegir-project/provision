<?php

/**
 * @file manual site migrate system
 */
function drush_provision_hostmaster_migrate_validate($site, $platform) {
  // XXX: this doesn't actually document the pre alpha9 upgrade path
  drush_print('
This command will operate the following changes in your system:

1. verify the server and the hostmaster site (provision-verify)
2. verify the new platform (which will download it if missing)
3. stop the queue (hosting-pause)
4. attempt a migration to the new platform (provision-migrate)
5. start the queue (hosting-resume)

We are making the following assumptions:
 * you have read UPGRADE.txt
 * you are executing this script as your "aegir" user
');

  if (!drush_confirm("Do you really want to proceed with the upgrade?")) {
    return drush_set_error('PROVISION_CANCEL_MIGRATE', 'Upgrade canceled by user');
  }

  $data = drush_get_merged_options();

  $alias_file = drush_server_home() . '/.drush/server_master.alias.drushrc.php';

  if (!file_exists($platform)) {
    $dir = realpath(dirname($platform));
    if (file_exists($dir)) {
      $platform = $dir . '/' . basename($platform);
      drush_log("changed relative platform path to absolute");
    }
  }

  // If the alias file exists, it means we are migrating from a post alpha9 release.
  if (file_exists($alias_file)) {
    provision_backend_invoke('@server_master', 'provision-verify');
    provision_backend_invoke('@hostmaster', 'provision-verify');

    drush_set_option('site_name', '@hostmaster');
    drush_set_option('old_platform', d('@hostmaster')->platform->name);

    $new_platform = '@platform_' . preg_replace("/[!\W]/", "", basename(rtrim($platform, '/')));
    drush_backend_invoke_args("provision-save", array($new_platform), array(
      'context_type' => 'platform',
      'server' => '@server_master',
      'web_server' => '@server_master',
      'root' => $platform,
      'makefile' => drush_get_option('makefile', dirname(__FILE__) . '/aegir.make'),
    ));
    provision_backend_invoke($new_platform, 'provision-verify');
    drush_set_option('new_platform', $new_platform);
    // exit if an error has occured. 
    if (drush_get_error()) {
      return false;
    }
  }
  else {

    // add a server record :
    $server = '@server_master';
    /**
     * @deprecated in drush3 it's 'options', in drush 4 it's 'cli', drop
     * 'options' when we drop drush3 support
     */
    $context = drush_get_context('cli') ? 'cli' : 'options';
    drush_backend_invoke_args("provision-save", array($server), array(
      'context_type' => 'server',
      'aegir_root' => dirname(rtrim($data['config_path'], '/')),
      'remote_host' => php_uname('n'),
      'script_user' => $data['script_user'],
     // apache or nginx or..
      'http_service_type' => drush_get_option('http_service_type', 'apache', $context),
      'web_group' => $data['web_group'],
      'master_url' => $data['master_url'],
      'restart_cmd' => $data['restart_cmd'],
     // mysql
      'db_service_type' => 'mysql',
      'master_db' => $data['master_db'],
    ));

    provision_backend_invoke($server, 'provision-verify');
    drush_set_option('server_name', $server);
    // exit if an error has occured. 
    if (drush_get_error()) {
      return false;
    }

   
    $old_platform = '@platform_' . preg_replace("/[!\W]/", "", basename(rtrim($data['publish_path'], '/')));
    drush_backend_invoke_args("provision-save", array($old_platform), array(
      'context_type' => 'platform',
      'server' => $server,
      'web_server' => $server,
      'root' => $data['publish_path'],
    ));
    provision_backend_invoke($old_platform, 'provision-verify');
    drush_set_option('old_platform', $old_platform);
    // exit if an error has occured. 
    if (drush_get_error()) {
      return false;
    }

    $new_platform = '@platform_' . preg_replace("/[!\W]/", "", basename(rtrim($platform, '/')));
    drush_backend_invoke_args("provision-save", array($new_platform), array(
      'context_type' => 'platform',
      'server' => $server,
      'web_server' => $server,
      'root' => $platform,
      'makefile' => drush_get_option('makefile', dirname(__FILE__) . '/aegir.make'),
    ));
    provision_backend_invoke($new_platform, 'provision-verify');
    drush_set_option('new_platform', $new_platform);
    // exit if an error has occured. 
    if (drush_get_error()) {
      return false;
    }


    $site_name = '@hostmaster';
    drush_backend_invoke_args("provision-save", array($site_name), array(
      'context_type' => 'site',
      'platform' => $old_platform,
      'db_server' => $server,
      'uri' => $site,
      'profile' => 'hostmaster',
    ));
    provision_backend_invoke($site_name, 'provision-verify');
    drush_set_option('site_name', $site_name);
    // exit if an error has occured. 
    if (drush_get_error()) {
      return false;
    }
  }
}

/**
 * Prepare the hostmaster site for migration
 *
 * @deprecated most of the stuff here needs to be removed in favor of
 * hosting-pause, but since that exists only starting with rc3, we
 * need to copy the content of it here. once 1.0 is released, we can
 * remove that code and just keep hosting-pause.
 */
function drush_provision_pre_hostmaster_migrate($site, $platform) {
  // wipe out cron entry
  exec('crontab -r');
  // we can't rely on update.php to run that because it will run too late. this can be removed in 0.5 and above
  provision_backend_invoke(drush_get_option('site_name'), 'sqlq', array("UPDATE {system} SET weight = 0 WHERE type='module' AND name='hosting';"));
}

function drush_provision_hostmaster_migrate($site, $platform) {
  provision_backend_invoke(drush_get_option('site_name'), 'provision-migrate', array(drush_get_option('new_platform')));
}

function drush_provision_post_hostmaster_migrate($site, $platform) {
  // we pass the context names we generated to the task so we can enforce that the names
  // stay the same.
  provision_backend_invoke(drush_get_option('site_name'), 'hostmaster-resume', array(), array(
    'old_platform_name' => drush_get_option('old_platform'),
    'new_platform_name' => drush_get_option('new_platform'),
  ));
}


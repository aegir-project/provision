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
  // add a server record :
  $server = '@server_localhost';
  drush_backend_invoke_args("provision-save", array($server), array(
    'context_type' => 'server',
   // files
    'file_service_type' => 'local',
    'aegir_root' => dirname(rtrim($data['config_path'], '/')),
    'config_path' => $data['config_path'],
    'backup_path' => $data['backup_path'],
    'script_user' => $data['script_user'],
   // apache 
    'http_service_type' => 'apache',
    'web_group' => $data['web_group'],
    'master_url' => $data['master_url'],
    'restart_cmd' => $data['restart_cmd'],
   // mysql
    'db_service_type' => 'mysql',
    'master_db' => $data['master_db'],

  ));
  provision_backend_invoke($server, 'provision-verify');
  drush_set_option('server_name', $server);

  $old_platform = '@platform_' . preg_replace("/[!\W]/", "", basename(rtrim($data['publish_path'], '/')));
  drush_backend_invoke_args("provision-save", array($old_platform), array(
    'context_type' => 'platform',
    'server' => $server,
    'web_server' => $server,
    'root' => $data['publish_path'],
  ));
  provision_backend_invoke($old_platform, 'provision-verify');
  drush_set_option('old_platform', $old_platform);

  $new_platform = '@platform_' . preg_replace("/[!\W]/", "", basename(rtrim($platform, '/')));
  drush_backend_invoke_args("provision-save", array($new_platform), array(
    'context_type' => 'platform',
    'server' => $server,
    'web_server' => $server,
    'root' => $platform,
  ));
  provision_backend_invoke($new_platform, 'provision-verify');
  drush_set_option('new_platform', $new_platform);

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
}


function drush_provision_pre_hostmaster_migrate($site, $platform) {
  if (!drush_confirm("This command will replace your crontab entry. continue?")) {
    return drush_set_error('PROVISION_CANCEL_MIGRATE', 'Cancelling');
  }
  // wipe out cron entry
  exec('crontab -r');

  // we can't rely on the park command to exist in pre-0.4 alpha 2 releases
  provision_backend_invoke(drush_get_option('site_name'), 'hostmaster-pause');
}

function drush_provision_hostmaster_migrate($site, $platform) {
  provision_backend_invoke(drush_get_option('site_name'), 'provision-migrate', drush_get_option('new_platform'));
}

function drush_provision_post_hostmaster_migrate($site, $platform) {
  // we pass the context names we generated to the task so we can enforce that the names
  // stay the same.
  provision_backend_invoke(drush_get_option('site_name'), 'hostmaster-resume', array(), array( 
    'old_platform_name' => drush_get_option('old_platform'),
    'new_platform_name' => drush_get_option('new_platform'),
  ));
}


<?php

/**
 * @file manual site migrate system
 */
function drush_provision_hostmaster_migrate_validate($site, $platform) {
  $data = drush_get_merged_options();
  // add a server record :
  $server = '@server_master';
  drush_backend_invoke_args("provision-save", array($server), array(
    'context_type' => 'server',
    'aegir_root' => dirname(rtrim($data['config_path'], '/')),
    'remote_host' => php_uname('n'),
    'script_user' => $data['script_user'],
   // apache or nginx or..
    'http_service_type' => drush_get_option('http_service_type', 'apache', 'options'),
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
    'makefile' => dirname(__FILE__) . '/aegir.make',
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


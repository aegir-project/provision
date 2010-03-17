<?php

class provisionService_http_apache extends provisionService {

  function templates() {
    $this->configs['vhost'] = 'vhost.tpl.php';
    $this->configs['vhost_redirect'] = 'vhost_redirect.tpl.php';
    $this->configs['vhost_disabled'] = 'vhost_disabled.tpl.php';
    $this->configs['platform'] = 'platform.tpl.php';
    $this->configs['server'] = 'server.tpl.php';
  }



  function verify($url) {
    if (PROVISION_CONTEXT_PLATFORM) {
      _provision_create_dir(drush_get_option('vhost_path'), dt("Virtual host configuration"), 0700);
      _provision_create_dir(drush_get_option('platform_conf_path'), dt("Platforms configuration"), 0700);
      
      if (drush_get_option('platform', null)) {
        $this->create_platform_config();
      }

      $this->create_server_config();
    }
    else {
      $this->create_site_config($url);
    }

    $this->parse_configs();
  }

  function parse_configs($cause_error = FALSE) {
    //This is required to be configurable, due to the fact that different hosts might need to do this differently.
    //TODO : test for this instead of relying on a configuration setting?
    $return = drush_shell_exec(escapeshellcmd(drush_get_option('restart_cmd')));
    if (!$return) {
      $msg = join("\n", drush_shell_exec_output());
      if ($cause_error) {
        return drush_set_error('PROVISION_WEB_RESTART_FAILED', dt("Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)", array("%msg" => $msg)));
      }
      else {
        drush_log(dt("Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)", array("%msg" => $msg)), "warning");
      }
    }
    else {
      drush_log(dt('Apache has been restarted'));
    }

    return $return;
  }



}

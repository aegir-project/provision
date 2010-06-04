<?php

class provisionService_file_remote extends provisionService_file {
  function init() {
    $this->remote_config_path = $this->config_path;
    $this->config_path = $this->config_path . '--' . $this->remote_host;
    parent::init();
  }

  function sync() {
    if (!drush_shell_exec('rsync -azC --delete %s/ %s@%s:%s', $this->config_path, $this->script_user, $this->remote_host, $this->remote_config_path)) {
      return drush_set_error('PROVISION_FILE_SYNC_FAILED', dt("Files could not be synced to remote server. Changes might not be available until this has been done. (error: %msg)", array("%msg" => join("\n", drush_shell_exec_output()))));
    }
    else {
      drush_log(dt('Files have been synced to remote server.'));
    }
  }
}

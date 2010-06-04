<?php

class provisionService_file_remote extends provisionService_file {
  function init() {
    $this->config_path = $this->config_path . '--' . $this->remote_host;
    parent::init();
  }

  function sync($path = NULL) {
    if (is_null($path)) {
      $path = $this->config_path;
    }
    if (drush_shell_exec('rsync -azC --delete %s/ %s@%s:%s', $path, $this->script_user, $this->remote_host, $path)) {
      drush_log(dt('@path has been synced to remote server.', array('@path' => $path)));
    }
    else {
      drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('Files could not be synced to remote server. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '%msg' => join("\n", drush_shell_exec_output()))));
    }
  }
}

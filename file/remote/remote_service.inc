<?php

require_once DRUSH_BASE_PATH . '/commands/core/rsync.core.inc';

class provisionService_file_remote extends provisionService_file {
  function init() {
    $this->config_path = $this->config_path . '--' . $this->remote_host;
    parent::init();
  }

  function sync($path = NULL, $exclude_sites = FALSE) {
    if (is_null($path)) {
      $path = $this->config_path;
    }

    $options = array(
      'delete' => TRUE,
      'exclude-sites' => $exclude_sites,
    );
    if (drush_core_call_rsync(escapeshellarg($path . '/'), escapeshellarg($this->script_user . '@' . $this->remote_host . ':' . $path), $options)) {
      drush_log(dt('@path has been synced to remote server.', array('@path' => $path)));
    }
    else {
      drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('Files could not be synced to remote server. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '%msg' => join("\n", drush_shell_exec_output()))));
    }
  }
}

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

    if ($this->exists($path)->status()) {
      $options = array(
        'relative' => TRUE,
        'delete' => TRUE,
      );
      if ($exclude_sites) {
        $options['exclude-sites'] = TRUE;
      }
      if (drush_core_call_rsync(escapeshellarg($path), escapeshellarg($this->script_user . '@' . $this->remote_host . ':/'), $options)) {
        drush_log(dt('@path has been synced to remote server.', array('@path' => $path)));
      }
      else {
        drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('Files could not be synced to remote server. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '%msg' => join("\n", drush_shell_exec_output()))));
      }
    }
    else { // File does not exist, remove it.
      if ($this->server()->shell_exec('rm -rf ' . escapeshellarg($path))) {
        drush_log(dt('@path has been removed from remote server.', array('@path' => $path)));
      }
      else {
        drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('Files could not be removed from remote server. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '%msg' => join("\n", drush_shell_exec_output()))));
      }
    }
  }
}

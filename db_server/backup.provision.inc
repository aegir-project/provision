<?php

function drush_provision_mysql_pre_provision_backup($url = NULL) {
  # set the umask to 077 so that the dump itself is generated so it's non-readable by the webserver
  umask(0077);
  drush_log("Generating mysql dump for $url.", 'backup');
  # mixed copy-paste of drush_shell_exec and provision_shell_exec
  $cmd = sprintf("mysqldump --defaults-file=/dev/fd/3 -rsites/%s/database.sql %s", escapeshellcmd($url), escapeshellcmd(drush_get_option('db_name'))); 
  drush_log($cmd);
  if (drush_get_context('DRUSH_VERBOSE') || drush_get_context('DRUSH_SIMULATE')) {
      drush_print('Executing: ' . $cmd, $indent);
  }

  if (drush_get_context('DRUSH_SIMULATE')) {
    return true;
  }

  # pipe handling code
  # we go through all this trouble to hide the password from the commandline, it's the most secure way (apart from writing a temporary file, which would create conflicts in parallel runs)
  $mycnf = sprintf('[client]
host=%s
user=%s
password=%s
', drush_get_option('db_host'), drush_get_option('db_user'), drush_get_option('db_passwd'));

  $descriptorspec = array(
   // 0 => array("pipe", "r"),  // this would be stdin, but we don't need to input into mysqldump
   1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
   2 => array("pipe", "w"),  // stderr is a file to write to
   3 => array("pipe", "r"),  // fd3 is our special file descriptor where we pass credentials
  );

  $process = proc_open($cmd, $descriptorspec, $pipes);

  $output = array();
  if (is_resource($process)) {
    fwrite($pipes[3], $mycnf);
    fclose($pipes[3]);

    $output = array_filter(array_merge(explode("\n", stream_get_contents($pipes[1])), explode("\n", stream_get_contents($pipes[2]))));
    // "It is important that you close any pipes before calling
    // proc_close in order to avoid a deadlock"
    fclose($pipes[1]);
    fclose($pipes[2]);
    $return_value = proc_close($process);
  } else {
    // XXX: failed to execute? unsure when this happens
    $return_value = -1;
  }

  # resuming drush_exec copy/paste
  $indent = 0;

  _drush_shell_exec_output_set($output);

  if (drush_get_context('DRUSH_VERBOSE')) {
    foreach ($output as $line) {
      drush_print($line, $indent + 2);
    }
  }

  $result = ($return_value == 0);
  if (!$result && !drush_get_option('force', false)) {
    drush_set_error('PROVISION_BACKUP_FAILED', dt("Could not generate database backup from mysqldump"));
  }
  # reset the umask to normal permissions
  umask(0022);
}

function drush_provision_mysql_pre_provision_backup_rollback($url = NULL) {
  provision_path("unlink", drush_get_option('sites_path') . "/$url/database.sql", TRUE, dt("Deleted mysql dump from sites directory"), 
  dt("Could not delete mysql dump from sites directory"));
}

function drush_provision_mysql_post_provision_backup($url = NULL) {
  drush_provision_mysql_pre_provision_backup_rollback($url);
}


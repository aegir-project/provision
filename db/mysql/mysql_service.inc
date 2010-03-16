<?php
// $Id$


// extends the pdo implementation
class provisionService_db_mysql extends provisionService_db_pdo {
  static public $PDO_type = 'mysql';


  function database_exists($name) {
    $result = $this->query("SHOW DATABASES LIKE '%s'", $name);
    return $result->fetchColumn(0);
  }


  function drop_database($name) {
    return $this->query("DROP DATABASE `%s`", $name);
  }


  function create_database($name) {
    return $this->query("CREATE DATABASE %s", $name);  
  }

  function can_create_database() {
    $test = drush_get_option('aegir_db_prefix', 'site_') .'test';
    $this->create_database($test);

    if ($this->database_exists($test)) {
      if (!$this->drop_database($test)) {
        drush_log(dt("Failed to drop database @dbname", array('@dbname' => $test)), 'warning');
      }
      return TRUE;
    }
    return FALSE;
  }
 

  function grant($name, $username, $password, $host = '') {
    $host = ($host) ? $host : '%';
    return $this->query("GRANT ALL PRIVILEGES ON `%s`.* TO `%s`@`%s` IDENTIFIED BY '%s'", $name, $username, $host, $password);
  }

  function revoke($name, $username, $host = '') {
      $host = ($host) ? $host : '%';
      $success = provision_db_query("REVOKE ALL PRIVILEGES ON `%s`.* FROM `%s`@`%s`", $name, $username, $host);
      
      // check if there are any privileges left for the user
      $grants = provision_db_query("SHOW GRANTS FOR `%s`@`%s`", $username, $host);
      $grant_found = FALSE;
      while ($grant = provision_db_fetch_array($grants)) {
        // those are empty grants: just the user line
        if (!preg_match("/^GRANT USAGE ON /", array_pop($grant))) {
          // real grant, we shouldn't remove the user
          $grant_found = TRUE;
          break;
        }
      }
      if (!$grant_found) {
        $success = provision_db_query("DROP USER `%s`@`%s`", $username, $host) && $success;
      }
      return $success;
  }


 

  function import_dump($dump_file, $creds) {
    extract($creds);

    $cmd = sprintf("mysql --defaults-file=/dev/fd/3 %s", escapeshellcmd($db_name));
    drush_log(sprintf("Importing database using command: %s", $cmd));
    # pipe handling code, this is inspired by drush_provision_mysql_pre_provision_backup()
    # we go through all this trouble to hide the password from the commandline, it's the most secure way (apart from writing a temporary file, which would create conflicts in parallel runs)
   $mycnf = sprintf('[client]
host=%s
user=%s
password=%s
', $db_host, $db_user, $db_passwd);

    $descriptorspec = array(
      0 => array("file", $dump_file, "r"),
      1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
      2 => array("pipe", "w"),  // stderr is a file to write to
      3 => array("pipe", "r"),  // fd3 is our special file descriptor where we pass credentials
    );
    $process = proc_open($cmd, $descriptorspec, $pipes);
    $output = "";
    if (is_resource($process)) {
      fwrite($pipes[3], $mycnf);
      fclose($pipes[3]);
  
      $output = stream_get_contents($pipes[1]) . stream_get_contents($pipes[2]);
      // "It is important that you close any pipes before calling
      // proc_close in order to avoid a deadlock"
      fclose($pipes[1]);
      fclose($pipes[2]);
      $return_value = proc_close($process);
    } else {
      // XXX: failed to execute? unsure when this happens
      $return_value = -1;
    }

    if ($return_value != 0) {
      drush_set_error('PROVISION_DB_IMPORT_FAILED', dt("Database import failed: %output", array('%output' => $output)));
    }
  }

  /**
   * Properly guess the host part of the MySQL username based on a given
   * database server host , web server IP and web server host.
   *
   * If the database and web server are the same machine, return a localhost
   * grant, which is a special and very common case which does not involve any
   * dns lookups and is thus faster and more secure.
   *
   * If the web server and database server are different machines, we prefer to
   * use the IP address, which is faster and more secure because fewer lookups
   * are done during connections.
   *
   */
  function grant_host($db_host, $web_ip, $web_host) {
    $result = provision_db_result(provision_db_query("select current_user()"));
    preg_match('/^.*@(.*)$/', $result, $matches);
    return $matches[1];
  }


  //todo - SORT THIS SHIT OUT!
  function generate_dump($url) {

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
  ', drush_get_option('db_host'), urldecode(drush_get_option('db_user')), urldecode(drush_get_option('db_passwd')));

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

}

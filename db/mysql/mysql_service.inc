<?php
// $Id$



// extends the pdo implementation
class provisionService_db_mysql extends provisionService_db_pdo {
   public $PDO_type = 'mysql';

   protected $has_port = TRUE;

   function default_port() {
     return 3306;
   }

  function database_exists($name) {
    $result = $this->query("SHOW DATABASES LIKE '%s'", $name);
    if ($result) {
      return $result->fetchColumn(0);
    }
  }


  function drop_database($name) {
    return $this->query("DROP DATABASE `%s`", $name);
  }


  function create_database($name) {
    return $this->query("CREATE DATABASE `%s`", $name);  
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
    $success = $this->query("REVOKE ALL PRIVILEGES ON `%s`.* FROM `%s`@`%s`", $name, $username, $host);

    // check if there are any privileges left for the user
    $grants = $this->query("SHOW GRANTS FOR `%s`@`%s`", $username, $host);
    $grant_found = FALSE;
    if ($grants) {
      while ($grant = $grants->fetch()) {
        // those are empty grants: just the user line
        if (!preg_match("/^GRANT USAGE ON /", array_pop($grant))) {
          // real grant, we shouldn't remove the user
          $grant_found = TRUE;
          break;
        }
      }
    }
    if (!$grant_found) {
      $success = $this->query("DROP USER `%s`@`%s`", $username, $host) && $success;
    }
    return $success;
  }


  function import_dump($dump_file, $creds) {
    extract($creds);

    $cmd = sprintf("mysql --defaults-file=/dev/fd/3 %s", escapeshellcmd($db_name));

    $success = $this->safe_shell_exec($cmd, $db_host, $db_user, $db_passwd, $dump_file);

    drush_log(sprintf("Importing database using command: %s", $cmd));

    if (!$success) {
      drush_set_error('PROVISION_DB_IMPORT_FAILED', dt("Database import failed: %output", array('%output' => $this->safe_shell_exec_output)));
    }
  }
  
  function grant_host(provisionContext_server $server) {
    $command = sprintf('mysql -u intntnllyInvalid -h %s -P %s',
      escapeshellarg($this->server->remote_host),
      escapeshellarg($this->server->db_port));


    $server->shell_exec($command);
    if (preg_match("/Access denied for user 'intntnllyInvalid'@'([^']*)'/", implode('', drush_shell_exec_output()), $match)) {
      return $match[1];
    }
    elseif (preg_match("/Host '([^']*)' is not allowed to connect to/", implode('', drush_shell_exec_output()), $match)) {
      return $match[1];
    }
    else {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Dummy connection failed to fail: %msg', array('%msg' => join("\n", drush_shell_exec_output()))));
    }
  }

  function generate_dump() {
    // Aet the umask to 077 so that the dump itself is generated so it's
    // non-readable by the webserver.
    umask(0077);
    // Mixed copy-paste of drush_shell_exec and provision_shell_exec.
    $cmd = sprintf('mysqldump --defaults-file=/dev/fd/3 -r%s/database.sql %s', escapeshellcmd(d()->site_path), escapeshellcmd(drush_get_option('db_name'))); 
    $success = $this->safe_shell_exec($cmd, drush_get_option('db_host'), urldecode(drush_get_option('db_user')), urldecode(drush_get_option('db_passwd')));

    if (drush_get_context('DRUSH_VERBOSE') || !$success) {
      foreach ($output as $line) {
        drush_print($line);
      }
    }

    if (!$success && !drush_get_option('force', false)) {
      drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not generate database backup from mysqldump. (error: %msg)', array('%msg' => $this->safe_shell_exec_output)));
    }
    // Reset the umask to normal permissions.
    umask(0022);
  }

  /**
   * We go through all this trouble to hide the password from the commandline,
   * it's the most secure way (apart from writing a temporary file, which would
   * create conflicts in parallel runs)
   */
  function safe_shell_exec($cmd, $db_host, $db_user, $db_passwd, $dump_file = null) {
   $mycnf = sprintf('[client]
host=%s
user=%s
password=%s
port=%s
', $db_host, $db_user, $db_passwd, $this->server->db_port);

   $stdin_spec = (!is_null($dump_file)) ? array("file", $dump_file, "r") : array("pipe", "r");

   $descriptorspec = array(
     0 => $stdin_spec,
     1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
     2 => array("pipe", "w"),  // stderr is a file to write to
     3 => array("pipe", "r"),  // fd3 is our special file descriptor where we pass credentials
   );
   $process = proc_open($cmd, $descriptorspec, $pipes);
   $this->safe_shell_exec_output = '';
   if (is_resource($process)) {
     fwrite($pipes[3], $mycnf);
     fclose($pipes[3]);

     $this->safe_shell_exec_output = stream_get_contents($pipes[1]) . stream_get_contents($pipes[2]);
     // "It is important that you close any pipes before calling
     // proc_close in order to avoid a deadlock"
     fclose($pipes[1]);
     fclose($pipes[2]);
     $return_value = proc_close($process);
   }
   else {
     // XXX: failed to execute? unsure when this happens
     $return_value = -1;
   }
   return ($return_value == 0);
  }
}

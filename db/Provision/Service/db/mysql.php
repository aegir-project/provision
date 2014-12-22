<?php
/**
 * @file
 * Provides the MySQL service driver.
 */

/**
 * The MySQL provision service.
 */
class Provision_Service_db_mysql extends Provision_Service_db_pdo {
  public $PDO_type = 'mysql';

  protected $has_port = TRUE;

  function default_port() {
    return 3306;
  }

  function drop_database($name) {
    return $this->query("DROP DATABASE `%s`", $name);
  }

  function create_database($name) {
    return $this->query("CREATE DATABASE `%s`", $name);
  }

  function can_create_database() {
    $test = drush_get_option('aegir_db_prefix', 'site_') . 'test';
    $this->create_database($test);

    if ($this->database_exists($test)) {
      if (!$this->drop_database($test)) {
        drush_log(dt("Failed to drop database @dbname", array('@dbname' => $test)), 'warning');
      }
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Verifies that provision can grant privileges to a user on a database.
   *
   * @return
   *   TRUE if the check was successful.
   */
  function can_grant_privileges() {
    $dbname   = drush_get_option('aegir_db_prefix', 'site_');
    $user     = $dbname . '_user';
    $password = $dbname . '_password';
    $host     = $dbname . '_host';
    if ($status = $this->grant($dbname, $user, $password, $host)) {
      $this->revoke($dbname, $user, $host);
    }
    return $status;
  }

  function grant($name, $username, $password, $host = '') {
    if (provision_file()->exists('/data/conf/clstr.cnf')->status()) {
      $host = '%';
    }
    $host = ($host) ? $host : '%';
    if ($host != "127.0.0.1") {
      $extra_host = "127.0.0.1";
      $success_extra_host = $this->query("GRANT ALL PRIVILEGES ON `%s`.* TO `%s`@`%s` IDENTIFIED BY '%s'", $name, $username, $extra_host, $password);
    }
    // Issue: https://github.com/omega8cc/provision/issues/2
    return $this->query("GRANT ALL PRIVILEGES ON `%s`.* TO `%s`@`%s` IDENTIFIED BY '%s'", $name, $username, $host, $password);
  }

  function revoke($name, $username, $host = '') {
    if (provision_file()->exists('/data/conf/clstr.cnf')->status()) {
      $host = '%';
    }
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

    if ($host != "127.0.0.1") {
      $extra_host = "127.0.0.1";
      $success_extra_host = $this->query("REVOKE ALL PRIVILEGES ON `%s`.* FROM `%s`@`%s`", $name, $username, $extra_host);

      // check if there are any privileges left for the user
      $grants = $this->query("SHOW GRANTS FOR `%s`@`%s`", $username, $extra_host);
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
        $success_extra_host = $this->query("DROP USER `%s`@`%s`", $username, $extra_host) && $success_extra_host;
      }
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

  function grant_host(Provision_Context_server $server) {
    $command = sprintf('mysql -u intntnllyInvalid -h %s -P %s -e "SELECT VERSION()"',
      escapeshellarg($this->server->remote_host),
      escapeshellarg($this->server->db_port));


    $server->shell_exec($command);
    $output = implode('', drush_shell_exec_output());
    if (preg_match("/Access denied for user 'intntnllyInvalid'@'([^']*)'/", $output, $match)) {
      return $match[1];
    }
    elseif (preg_match("/Host '([^']*)' is not allowed to connect to/", $output, $match)) {
      return $match[1];
    }
    elseif (preg_match("/ERROR 2003 \(HY000\): Can't connect to MySQL server on/", $output, $match)) {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Connection to database server failed: %msg', array('%msg' => join("\n", drush_shell_exec_output()))));
    }
    else {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Dummy connection failed to fail. Either your MySQL permissions are too lax, or the response was not understood. See http://is.gd/Y6i4FO for more information. %msg', array('%msg' => join("\n", drush_shell_exec_output()))));
    }
  }

  function generate_dump() {
    // Aet the umask to 077 so that the dump itself is generated so it's
    // non-readable by the webserver.
    umask(0077);
    // Mixed copy-paste of drush_shell_exec and provision_shell_exec.
    $cmd = sprintf("mysqldump --defaults-file=/dev/fd/3 --single-transaction --quick --no-autocommit --default-character-set=utf8 --hex-blob %s | sed 's|/\\*!50001 CREATE ALGORITHM=UNDEFINED \\*/|/\\*!50001 CREATE \\*/|g; s|/\\*!50017 DEFINER=`[^`]*`@`[^`]*`\s*\\*/||g' | sed '/\\*!50013 DEFINER=.*/ d' > %s/database.sql", escapeshellcmd(drush_get_option('db_name')), escapeshellcmd(d()->site_path));
    $success = $this->safe_shell_exec($cmd, drush_get_option('db_host'), urldecode(drush_get_option('db_user')), urldecode(drush_get_option('db_passwd')));

    $dump_size_too_small = filesize(d()->site_path . '/database.sql') < 1024;
    if ((!$success || $dump_size_too_small) && !drush_get_option('force', FALSE)) {
      drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not generate database backup from mysqldump. (error: %msg)', array('%msg' => $this->safe_shell_exec_output)));
    }
    // Reset the umask to normal permissions.
    umask(0022);
  }

  /**
   * We go through all this trouble to hide the password from the commandline,
   * it's the most secure way (apart from writing a temporary file, which would
   * create conflicts in parallel runs)
   *
   * XXX: this needs to be refactored so it:
   *  - works even if /dev/fd/3 doesn't exit
   *  - has a meaningful name (we're talking about reading and writing
   * dumps here, really, or at least call mysql and mysqldump, not
   * just any command)
   *  - can be pushed upstream to drush (http://drupal.org/node/671906)
   */
  function safe_shell_exec($cmd, $db_host, $db_user, $db_passwd, $dump_file = NULL) {
    $mycnf = sprintf('[client]
host=%s
user=%s
password="%s"
port=%s
', $db_host, $db_user, $db_passwd, $this->server->db_port);

    $stdin_spec = (!is_null($dump_file)) ? array("file", $dump_file, "r") : array("pipe", "r");

    $descriptorspec = array(
      0 => $stdin_spec,
      1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
      2 => array("pipe", "w"),  // stderr is a file to write to
      3 => array("pipe", "r"),  // fd3 is our special file descriptor where we pass credentials
    );
    $pipes = array();
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

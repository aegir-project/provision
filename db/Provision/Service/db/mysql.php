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
    $host = ($host) ? $host : '%';
    if ($host != "127.0.0.1") {
      $extra_host = "127.0.0.1";
      $success_extra_host = $this->query("GRANT ALL PRIVILEGES ON `%s`.* TO `%s`@`%s` IDENTIFIED BY '%s'", $name, $username, $extra_host, $password);
    }
    // Issue: https://github.com/omega8cc/provision/issues/2
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
    elseif (preg_match("/ERROR 2002 \(HY000\): Can't connect to local MySQL server through socket '([^']*)'/", $output, $match)) {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Local database server not running, or not accessible via socket (%socket): %msg', array('%socket' => $match[1], '%msg' => join("\n", drush_shell_exec_output()))));
    }
    elseif (preg_match("/ERROR 2003 \(HY000\): Can't connect to MySQL server on/", $output, $match)) {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Connection to database server failed: %msg', array('%msg' => join("\n", drush_shell_exec_output()))));
    }
    elseif (preg_match("/ERROR 2005 \(HY000\): Unknown MySQL server host '([^']*)'/", $output, $match)) {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Cannot resolve database server hostname (%host): %msg', array('%host' => $match[1], '%msg' => join("\n", drush_shell_exec_output()))));
    }
    else {
      return drush_set_error('PROVISION_DB_CONNECT_FAIL', dt('Dummy connection failed to fail. Either your MySQL permissions are too lax, or the response was not understood. See http://is.gd/Y6i4FO for more information. %msg', array('%msg' => join("\n", drush_shell_exec_output()))));
    }
  }

  /**
   * Generate the contents of a mysql config file containing database
   * credentials.
   */
  function generate_mycnf($db_host = NULL, $db_user = NULL, $db_passwd = NULL, $db_port = NULL) {
    // Look up defaults, if no credentials are provided.
    if (is_null($db_host)) {
      $db_host = drush_get_option('db_host');
    }
    if (is_null($db_user)) {
      $db_user = urldecode(drush_get_option('db_user'));
    }
    if (is_null($db_passwd)) {
      $db_passwd = urldecode(drush_get_option('db_passwd'));
    }
    if (is_null($db_port)) {
      $db_port = $this->server->db_port;
    }

    $mycnf = sprintf('[client]
host=%s
user=%s
password="%s"
port=%s
', $db_host, $db_user, $db_passwd, $db_port);

    return $mycnf;
  }

  /**
   * Generate the descriptors necessary to open a process with readable and
   * writeable pipes.
   */
  function generate_descriptorspec($stdin_file = NULL) {
    $stdin_spec = is_null($stdin_file) ? array("pipe", "r") : array("file", $stdin_file, "r");
    $descriptorspec = array(
      0 => $stdin_spec,         // stdin is a pipe that the child will read from
      1 => array("pipe", "w"),  // stdout is a pipe that the child will write to
      2 => array("pipe", "w"),  // stderr is a file to write to
      3 => array("pipe", "r"),  // fd3 is our special file descriptor where we pass credentials
    );
    return $descriptorspec;
  }

  /**
   * Return an array of regexes to filter lines of mysqldumps.
   */
  function get_regexes() {
    static $regexes = NULL;
    if (is_null($regexes)) {
      $regexes = array(
        // remove DEFINER entries
        '#/\*!50013 DEFINER=.*/#' => FALSE,
        // remove another kind of DEFINER line
        '#/\*!50017 DEFINER=`[^`]*`@`[^`]*`\s*\*/#' => '',
        // remove broken CREATE ALGORITHM entries
        '#/\*!50001 CREATE ALGORITHM=UNDEFINED \*/#' => "/*!50001 CREATE */",
      );

      // Allow regexes to be altered or appended to.
      drush_command_invoke_all_ref('provision_mysql_regex_alter', $regexes);
    }
    return $regexes;
  }

  function filter_line(&$line) {
    $regexes = $this->get_regexes();
    foreach ($regexes as $find => $replace) {
      if ($replace === FALSE) {
        if (preg_match($find, $line)) {
          // Remove this line entirely.
          $line = FALSE;
        }
      }
      else {
        $line = preg_replace($find, $replace, $line);
        if (is_null($line)) {
          // preg exploded in our face, oops.
          drush_set_error('PROVISION_BACKUP_FAILED', dt(
            "Error while running regular expression:\n Pattern: !find\n Replacement: !replace",
            array(
              '!find' => $find,
              '!replace' => $replace,
          )));
        }
      }
    }
  }

  /**
   * Generate a mysqldump for use in backups.
   */
  function generate_dump() {
    // Set the umask to 077 so that the dump itself is non-readable by the
    // webserver.
    umask(0077);

    // If a database uses Global Transaction IDs (GTIDs), information about this is written to the dump
    // file by default.  Trying to import such a dump during a clone or migrate will fail.  So use the
    // '--set-gtid-purged=OFF' option to suppress the restoration of GTIDs.  GTIDs were added in MySQL version 5.6
    if (drush_get_option('provision_mysqldump_suppress_gtid_restore', FALSE)) {
      $gtid_option = '--set-gtid-purged=OFF';
    } // if
    else {
      $gtid_option = '';
    } // else

    // Mixed copy-paste of drush_shell_exec and provision_shell_exec.
    $cmd = sprintf("mysqldump --defaults-file=/dev/fd/3 %s --single-transaction --quick --no-autocommit %s", $gtid_option, escapeshellcmd(drush_get_option('db_name')));

    // Fail if db file already exists.
    $dump_file = fopen(d()->site_path . '/database.sql', 'x');
    if ($dump_file === FALSE) {
      drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not write database backup file mysqldump'));
    }
    else {
      $pipes = array();
      $descriptorspec = $this->generate_descriptorspec();
      $process = proc_open($cmd, $descriptorspec, $pipes);
      if (is_resource($process)) {
        fwrite($pipes[3], $this->generate_mycnf());
        fclose($pipes[3]);

        // At this point we have opened a pipe to that mysqldump command. Now
        // we want to read it one line at a time and do our replacements.
        while (($buffer = fgets($pipes[1], 4096)) !== FALSE) {
          $this->filter_line($buffer);
          // Write the resulting line in the backup file.
          if ($buffer && fwrite($dump_file, $buffer) === FALSE) {
            drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not write database backup file mysqldump'));
          }
        }
        // Close stdout.
        fclose($pipes[1]);
        // Catch errors returned by mysqldump.
        $err = fread($pipes[2], 4096);
        // Close stderr as well.
        fclose($pipes[2]);
        if (proc_close($process) != 0) {
          drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not write database backup file mysqldump (command: %command) (error: %msg)', array('%msg' => $err, '%command' => $cmd)));
        }
      }
      else {
        drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not run mysqldump for backups'));
      }
    }

    $dump_size_too_small = filesize(d()->site_path . '/database.sql') < 1024;
    if (($dump_size_too_small) && !drush_get_option('force', FALSE)) {
      drush_set_error('PROVISION_BACKUP_FAILED', dt('Could not generate database backup from mysqldump. (error: %msg)', array('%msg' => $err)));
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
   *  - works even if /dev/fd/3 doesn't exist
   *  - has a meaningful name (we're talking about reading and writing
   * dumps here, really, or at least call mysql and mysqldump, not
   * just any command)
   *  - can be pushed upstream to drush (http://drupal.org/node/671906)
   */
  function safe_shell_exec($cmd, $db_host, $db_user, $db_passwd, $dump_file = NULL) {
    $mycnf = $this->generate_mycnf($db_host, $db_user, $db_passwd);
    $descriptorspec = $this->generate_descriptorspec($dump_file);
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

  function utf8mb4_is_supported() {
    // Ensure that provision can connect to the database.
    if (!$this->connect()) {
      return FALSE;
    }

    // Ensure that the MySQL driver supports utf8mb4 encoding.
    $version = $this->conn->getAttribute(PDO::ATTR_CLIENT_VERSION);
    if (strpos($version, 'mysqlnd') !== FALSE) {
      // The mysqlnd driver supports utf8mb4 starting at version 5.0.9.
      $version = preg_replace('/^\D+([\d.]+).*/', '$1', $version);
      if (version_compare($version, '5.0.9', '<')) {
        return FALSE;
      }
    }
    else {
      // The libmysqlclient driver supports utf8mb4 starting at version 5.5.3.
      if (version_compare($version, '5.5.3', '<')) {
        return FALSE;
      }
    }

    // Ensure that the MySQL server supports large prefixes and utf8mb4.
    $dbname = uniqid(drush_get_option('aegir_db_prefix', 'site_'));
    $this->create_database($dbname);
    $success = $this->query("CREATE TABLE `%s`.`drupal_utf8mb4_test` (id VARCHAR(255), PRIMARY KEY(id(255))) CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci ROW_FORMAT=DYNAMIC", $dbname);
    if (!$this->drop_database($dbname)) {
      drush_log(dt("Failed to drop database @dbname", array('@dbname' => $dbname)), 'warning');
    }

    return ($success !== FALSE);
  }
}

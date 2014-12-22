<?php

/**
 * @file Provision named context server class.
 */

/**
 * Server context class.
 *
 * This class bootstraps the Service API by generating server
 * objects for each of the available service types.
 */
class Provision_Context_server extends Provision_Context {
  /**
   * Associative array of services for this server.
   *
   * @see Provision_Service
   */
  protected $services = array();

  static function option_documentation() {
    $options = array(
      'remote_host' => 'server: host name; default localhost',
      'script_user' => 'server: OS user name; default current user',
      'aegir_root' => 'server: Aegir root; default ' . getenv('HOME'),
      'master_url' => 'server: Hostmaster URL',
    );
    foreach (drush_command_invoke_all('provision_services') as $service => $default) {
      // TODO: replace this file scanning nastiness, with a hook!
      $reflect = new reflectionClass('Provision_Service_' . $service);
      $base_dir = dirname($reflect->getFilename());
      $types = array();
      $options[$service . '_service_type'] = 'placeholder';
      foreach (array_keys(drush_scan_directory($base_dir, '%.*_service\.inc%')) as $service_file) {
        if (preg_match('%^' . $base_dir . '/([a-z]+)/(?:\1)_service.inc$%', $service_file, $match)) {
          $types[] = $match[1];
          include_once($service_file);
          $options = array_merge($options, call_user_func(array(sprintf('Provision_Service_%s_%s', $service, $match[1]), 'option_documentation')));
        }
      }
      $options[$service . '_service_type'] = 'server: ' . implode(', ', $types) . ', or null; default ' . (empty($default) ? 'null' : $default);
    }
    return $options;
  }

  function init_server() {

    $this->setProperty('remote_host', 'localhost');
    if ($this->name == '@server_master') {
      $this->setProperty('aegir_root', getenv('HOME'));
      $this->setProperty('script_user', provision_current_user());
    }
    else {
      $this->aegir_root = d('@server_master')->aegir_root;
      // In certain cicumstances it might be useful to have different
      // script_users on different Aegir servers, but this could also cause
      // weird things to happen, so use with caution!
      $this->setProperty('script_user', d('@server_master')->script_user);
    }

    $this->setProperty('ip_addresses', array(), true);

    $this->backup_path = $this->aegir_root . '/backups';
    $this->config_path = $this->aegir_root . '/config/' . ltrim($this->name, '@');
    $this->include_path = $this->aegir_root . '/config/includes';
    $this->clients_path = $this->aegir_root . '/clients';

    $this->setProperty('master_url');
    $this->setProperty('admin_email', 'admin@' . $this->remote_host);
    $this->load_services();
  }

  /**
   * Iterate through the available service types and spawn a handler for each type.
   */
  function load_services() {
    $service_list = drush_command_invoke_all('provision_services');
    foreach ($service_list as $service => $default) {
      $this->spawn_service($service, $default);
    }
  }

  /**
   * Spawn an instance for a specific service type and associate it to the owner.
   */
  function spawn_service($service, $default = null) {
    $type_option = "{$service}_service_type";

    $type = isset($this->options[$type_option]) ? $this->options[$type_option] : $default;
    if ($service === 'file') {
      // Force provision-save local
      $command = drush_get_command();
      if (preg_match("/^provision-save\b/", $command['command'])) {
        $type = 'local';
      }
    }
    if ($type) {
      $className = sprintf("Provision_Service_%s_%s", $service, $type);
      if (class_exists($className)) {
        drush_log("Loading $type driver for the $service service");
        $object = new $className($this->name);
        $this->services[$service] = $object;
        $this->setProperty($type_option, $type);
      }
      else {
        drush_log("Unable to load $type driver for the $service service", 'error');
      }
    }
    else {
      drush_log("Driver type not specified for the $service service, provide it with --{$service}_service_type");
      $this->services[$service] = new Provision_Service_null($this->name);
    }
  }

  /**
   * Retrieve a service of a specific type from the context.
   */
  function service($service, $name = null) {
    $this->services[$service]->setContext(($name) ? $name : $this->name);
    return $this->services[$service];
  }

  /**
   * Retrieve a list of service objects associated with this server.
   */
  function get_services() {
    $services = array();
    foreach ($this->services as $service => $object) {
      $services[$service] = $this->name;
    }
    return $services;
  }


  function verify() {
    $this->type_invoke('verify');
  }

  /**
   * Execute $command on this server, using SSH if necessary.
   *
   * @param $command
   *   Shell command to execute.
   *
   * @return
   *   Same as drush_shell_exec(). Use drush_shell_exec_output() for standard
   *   out and error.
   */
  function shell_exec($command) {
    if (provision_is_local_host($this->remote_host)) {
      return drush_shell_exec(escapeshellcmd($command));
    }
    else {
      return drush_shell_exec('ssh ' . drush_get_option('ssh-options', '-o PasswordAuthentication=no') . ' %s %s', $this->script_user . '@' . $this->remote_host, escapeshellcmd($command));
    }
  }

  /**
   * If necessary, sync files out to a remote server.
   *
   * @param $path
   *   Full path to sync.
   * @param $additional_options
   *   An array of options that overrides whatever was passed in on the command
   *   line (like the 'process' context, but only for the scope of this one
   *   call).
   */
  function sync($path = NULL, $additional_options = array()) {
    if (!provision_is_local_host($this->remote_host)) {
      if (is_null($path)) {
        $path = $this->config_path;
      }

      if (provision_file()->exists($path)->status()) {
        $default_options = array(
          'relative' => TRUE,
          'keep-dirlinks' => TRUE,
          'omit-dir-times' => TRUE,
        );
        $global_extra_options = drush_get_option('global_sync_options', array());
        $options = array_merge($default_options, $additional_options, $global_extra_options);


        // We need to do this due to how drush creates the rsync command.
        // If the option is present at all , even if false or null, it will
        // add it to the command.
        if (!isset($additional_options['no-delete']) || $additional_options['no-delete'] == FALSE ) {
          $options['delete'] = TRUE;
        }

        if (drush_core_call_rsync(escapeshellarg($path), escapeshellarg($this->script_user . '@' . $this->remote_host . ':/'), $options, TRUE, FALSE)) {
          drush_log(dt('@path has been synced to remote server @remote_host.', array('@path' => $path, '@remote_host' => $this->remote_host)));
        }
        else {
          drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('@path could not be synced to remote server @remote_host. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '@remote_host' => $this->remote_host, '%msg' => join("\n", drush_shell_exec_output()))));
        }
      }
      else { // File does not exist, remove it.
        if ($this->shell_exec('rm -rf ' . escapeshellarg($path))) {
          drush_log(dt('@path has been removed from remote server @remote_host.', array('@path' => $path, '@remote_host' => $this->remote_host)));
        }
        else {
          drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('@path could not be removed from remote server @remote_host. Changes might not be available until this has been done. (error: %msg)', array('@path' => $path, '@remote_host' => $this->remote_host, '%msg' => join("\n", drush_shell_exec_output()))));
        }
      }
    }
  }

  /**
   * If necessary, fetch file from a remote server.
   *
   * @param $path
   *   Full path to fetch.
   * @param $additional_options
   *   An array of options that overrides whatever was passed in on the command
   *   line (like the 'process' context, but only for the scope of this one
   *   call).
   */
  function fetch($path, $additional_options = array()) {
    if (!provision_is_local_host($this->remote_host)) {
      if (provision_file()->exists($path)->status()) {
        $options = array_merge(array(
          'omit-dir-times' => TRUE,
        ), $additional_options);

        // We need to do this due to how drush creates the rsync command.
        // If the option is present at all, even if false or null, it will
        // add it to the command.
        if (!isset($additional_options['no-delete']) || $additional_options['no-delete'] == FALSE ) {
          $options['delete'] = TRUE;
        }

        if (drush_core_call_rsync(escapeshellarg($this->script_user . '@' . $this->remote_host . ':/') . $path, $path, $options, TRUE, FALSE)) {
          drush_log(dt('@path has been fetched from remote server @remote_host.', array(
            '@path' => $path,
            '@remote_host' => $this->remote_host))
          );
        }
        else {
          drush_set_error('PROVISION_FILE_SYNC_FAILED', dt('@path could not be fetched from remote server @remote_host.' .
            ' Changes might not be available until this has been done. (error: %msg)', array(
              '@path' => $path,
              '@remote_host' => $this->remote_host,
              '%msg' => join("\n", drush_shell_exec_output())))
          );
        }
      }
    }
  }

}


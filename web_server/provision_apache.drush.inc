<?php
// $Id$
/**
 * @file
 * Apache provisioning module
 * This module simply serves to generate the virtual host entry, and make sure apache gets reloaded properly.
 * Because Drupal is running via the command line for the entirety of this process, it is only necessary to make
 * it available online once everything has been completed.
 *
 * This module still requires configuration and sanity checks. Need to figure out a way to inspect the apache configuration, 
 * to ensure that the sites are getting loaded up correctly.
 */

/**
 * Implementation of hook_drush_init()
 *
 * This sets a few default drush options by automatically guessing
 * proper defaults and settings based on the environment.
 */
function provision_apache_drush_init() {
  $command = drush_get_command();
  $command = explode(" ", $command['command']);
  if (preg_match("/^provision-/", $command[0])) {
    // Set up defines for platform
    $docroot = drush_get_option(array("r", "root"), $_SERVER['PWD']);

    $path = drush_set_default('docroot_path', rtrim(($docroot) ? $docroot : $_SERVER['DOCUMENT_ROOT'], '/'));
    drush_set_default('sites_path', $path . '/sites');
    drush_set_default('publish_path', realpath(drush_get_option('docroot_path')));

    $parts = explode("/", $path);
    array_pop($parts);
    $parent_path = drush_set_default('parent_path', implode("/" , $parts));

    drush_set_default('backup_path', $parent_path . '/backups');
    drush_set_default('config_path', $parent_path . '/config');

    $config_path = drush_get_option('config_path');
    drush_set_default('vhost_path', $config_path . '/vhost.d');
    drush_set_default('platform_conf_path', $config_path . '/platform.d');

    // Commands
    drush_set_default('restart_cmd', _provision_default_restart_cmd());

    // System account
    drush_set_default('web_group', _provision_default_web_group());
    drush_set_default('script_user', get_current_user());

    // Redirection urls
    drush_set_default('master_url', $GLOBALS['base_url']);
    $master_url = drush_get_option('master_url');
    drush_set_default('web_disable_url', $master_url .'/hosting/disabled');
    drush_set_default('web_maintenence_url', $master_url .'/hosting/maintenance');

    drush_set_default('web_ip', '127.0.0.1');
    drush_set_default('web_port', 80);
  }
}

/**
 * The default template provided for the virtual host configuration
 */
function _provision_apache_default_template() {
  return file_get_contents(dirname(__FILE__) ."/provision_apache_vhost.tpl.php");
}

/**
 * The template for site redirection
 */
function _provision_apache_redirect_template() {
  return file_get_contents(dirname(__FILE__) ."/provision_apache_vhost_redirect.tpl.php");
}

/**
 * The default template for disabled sites
 */
function _provision_apache_disabled_template() {
  return file_get_contents(dirname(__FILE__) ."/provision_apache_vhost_disabled.tpl.php");
}

/**
 * The template for platform configuration settings
 */
function _provision_apache_platform_template() {
  return file_get_contents(dirname(__FILE__) ."/provision_apache_platform.tpl.php");
}

/**
 * Delete virtual host file
 */
function _provision_apache_delete_vhost_config($url) {
  // backward compatibility with 0.3
  if (provision_path_exists(drush_get_option('vhost_path') .'/'. $url)) {
    provision_path("unlink", drush_get_option('vhost_path') ."/" . $url, TRUE, dt("Removed apache virtual host configuration"));
  } else {
    $options = drush_get_merged_options();
    // backward compatibility with 0.3
    if (!$options['site_port'] || $options['site_port'] < 1 || $options['site_port'] > 65536) { 
      $options['site_port'] = 80;
    }
    $file = $url . '_' . $options['site_port'];
    provision_path("unlink", drush_get_option('vhost_path') ."/" . $file, TRUE, dt("Removed apache virtual host configuration"));
  }
  drush_command_invoke_all('provision_apache_delete_vhost', $url, $options);
}

/**
 * Generate virtual host file
 *
 * This will create a VirtualHost configuration file for the domain
 * $url. This is really a stub to provision_write_config() that sets up the
 * options properly, guesses the right template and fires a hook to
 * allow other modules to add lines to the default config, using
 * hook_provision_apache_vhost_config(). Parameters in the template
 * are expanded based on the options available through
 * drush_get_merged_options().
 *
 * @param $url
 *    the domain to generate the configuration file for
 * @param $template
 *    the template to use, if NULL, the default will be used
 *
 * @see _provision_apache_default_template()
 * @see _provision_apache_redirect_template()
 * @see provision_write_config()
 * @see hook_provision_apache_vhost_config()
 * @see drush_get_merged_options()
 */
function _provision_apache_create_vhost_config($url, $template = NULL) {
  $options = drush_get_merged_options();
  // backward compatibility with 0.3
  if (!$options['site_port'] || $options['site_port'] < 1 || $options['site_port'] > 66535) { 
    $options['site_port'] = 80;
  }
  if (is_null($template)) {
    $template = _provision_apache_default_template();
  }
  if (!empty($options['redirection'])) {
    $template .= _provision_apache_redirect_template();
  }
  if ($options['aliases'] && !is_array($options['aliases'])) {
    $options['aliases'] = explode(",", $options['aliases']);
  }

  $options['extra_config'] = "# Extra configuration from modules:\n";
  $options['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_vhost_config', $url, $options));
  /* one file per virtualhost name/port combination
   *
   * the rationale here is that we can have different sites on
   * different ports that will be generated at different times, so we
   * will not be able to generate a full config file for all ports in
   * one pass
   */
  $file = $url . '_' . $options['site_port'];
  // backward compatibility with 0.3
  if (provision_path_exists(drush_get_option('vhost_path') .'/'. $url)) {
    rename(drush_get_option('vhost_path') .'/'. $url, drush_get_option('vhost_path') .'/'. $file);
  }
  drush_log(dt("Generating apache host configuration file %file.", array('%file' => $file)));
  $writable = provision_path("writable", drush_get_option('vhost_path'), TRUE , NULL,
      dt("Virtual host configuration path @path is not writable."), 
      'PROVISION_VHOST_PATH_NOT_WRITABLE');
  
  if ($writable) {
    return provision_write_config(drush_get_option('vhost_path') .'/'. $file, $template, $options);
  } else {
    return $writable;
  }
}

/**
 * Create a platform-level Apache cofniguration file
 *
 * This creates a config file for the <Directory> the platform is in
 *
 * most of what is done here is add the relevant .htaccess directives to
 * the platform configuration.
 */
function _provision_apache_create_platform_config($url) {
  // only create a platform level config if the platform is specified
  $data = drush_get_merged_options();
  $options = drush_get_merged_options();
  $data['extra_config'] = "# Extra configuration from modules:\n";
  $data['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_dir_config', $data));
  $file = 'platform_' . $options['platform'] .  '.conf';
  drush_log(dt("Generating apache platform configuration file %file.", array('%file' => $file)));
  $writable = provision_path("writable", drush_get_option('platform_conf_path'), TRUE , NULL, dt("Platform configuration path @path is not writable."), 'PROVISION_VHOST_PATH_NOT_WRITABLE');

  if ($writable) {
    return provision_write_config(drush_get_option('platform_conf_path') . '/' . $file, _provision_apache_platform_template(), $data);
  } else {
    return $writable;
  }
}

/**
 * Create a system-wide Apache configuration file.
 *
 * This creates a configuration file without any <VirtualHost> headers
 * so that the configuration applies to all VirtualHost (in
 * general). It calls hook_provision_apache_server_config() to allow
 * third party extensions to add stuff to the configuration file.
 *
 * @see _provision_apache_default_server_template()
 * @see provision_write_config()
 */
function _provision_apache_create_server_config($url) {
  $options = drush_get_merged_options();
  $template = _provision_apache_default_server_template();
  
  $options['extra_config'] = "# Extra configuration from modules:\n";
  $options['extra_config'] .= join("\n", drush_command_invoke_all('provision_apache_server_config', $url, $options));

  $file = drush_get_option('config_path') .'/apache.conf';
  drush_log(dt("Generating apache server configuration file %file.", array('%file' => $file)));
  $writable = provision_path("writable", drush_get_option('config_path'), TRUE , NULL,
      dt("Virtual host configuration path @path is not writable."), 
      'PROVISION_VHOST_PATH_NOT_WRITABLE');
  
  if ($writable) {
    return provision_write_config($file, $template, $options);
  } else {
    return $writable;
  }
}

/**
 * default template for server-wide configurations
 * 
 * @see _provision_apache_create_server_config()
 */
function _provision_apache_default_server_template() {
    return file_get_contents(dirname(__FILE__) ."/provision_apache_server.tpl.php");
}

function _provision_apache_delete_platform_config($file) {
  provision_path("unlink", drush_get_option('platform_conf_path') ."/" . $file, TRUE, dt("Removed apache platform vhost configuration"));
}



/**
 * Restart Apache
 */
function _provision_apache_restart_apache($cause_error = FALSE) {
  //This is required to be configurable, due to the fact that different hosts might need to do this differently.
  //TODO : test for this instead of relying on a configuration setting?
  $return = drush_shell_exec(escapeshellcmd(drush_get_option('restart_cmd')));
  if (!$return) {
    $msg = join("\n", drush_shell_exec_output());
    if ($cause_error) {
      return drush_set_error('PROVISION_WEB_RESTART_FAILED', dt("Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)", array("%msg" => $msg)));
    }
    else {
      drush_log(dt("Web server could not be restarted. Changes might not be available until this has been done. (error: %msg)", array("%msg" => $msg)), "warning");
    }
  }
  else {
    drush_log(dt('Apache has been restarted'));
  }

  return $return;
}


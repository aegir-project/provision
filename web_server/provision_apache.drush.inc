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

function provision_apache_provision_init() {
  // Set up defines for platform
  $docroot = drush_get_option(array("r", "root"), $_SERVER['PWD']);

  $path = drush_set_default('docroot_path', rtrim(($docroot) ? $docroot : $_SERVER['DOCUMENT_ROOT'], '/'));
  drush_set_default('sites_path', $path . '/sites');
  drush_set_default('publish_path', drush_get_option('docroot_path'));

  $parts = explode("/", $path);
  array_pop($parts);
  $parent_path = drush_set_default('parent_path', implode("/" , $parts));


  drush_set_default('backup_path', $parent_path . '/backups');
  drush_set_default('config_path', $parent_path . '/config');

  $config_path = drush_get_option('config_path');
  drush_set_default('vhost_path', $config_path . '/vhost.d');

  // Commands
  drush_set_default('restart_cmd', _provision_default_restart_cmd());

  // System account
  drush_set_default('web_group', _provision_default_web_group());
  drush_set_default('script_user', get_current_user());

  // Redirection urls
  drush_set_default('master_url', $GLOBALS['base_url']);
  $master_url = drush_get_option('master_url');
  drush_set_default('web_disable_url', $master_url .'/provision/disabled');
  drush_set_default('web_maintenence_url', $master_url .'/provision/maintenance');

  drush_set_default('web_ip', '127.0.0.1');
  drush_set_default('web_port', 80); 
}

/**
 * The default template provided for the virtual host configuration
 */
function _provision_apache_default_template() {
  return file_get_contents(dirname(__FILE__) ."/provision_apache_vhost.tpl.php");
}

/**
 * The default template for disabled sites
 */
function _provision_apache_redirect_template() {
  return file_get_contents(dirname(__FILE__) ."/provision_apache_vhost_redirect.tpl.php");
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
  provision_path("unlink", drush_get_option('vhost_path') ."/$url", TRUE, dt("Removed apache virtual host configuration"));
}

/**
 * Generate virtual host file
 */
function _provision_apache_create_vhost_config($url, $template = NULL) {
  $options = drush_get_merged_options();
  if (is_null($template)) {
    $template = _provision_apache_default_template();
  }
  $options['extra_config'] = "# Extra configuration from modules:\n";
  foreach (drush_command_implements('provision_vhost_config') as $module) {
    $options['extra_config'] .= "# -- $module settings --\n";
    $options['extra_config'] .= module_invoke($module, 'provision_vhost_config', $url, $options) . "\n";
  }
  return _provision_apache_create_config($url, $options, $template);
}

/**
 * Write an apache configuration file
 */
function _provision_apache_create_config($file, $data, $template) {
  drush_log(dt("Generating apache host configuration file %file.", array('%file' => $file)));
  $writable = provision_path("writable", drush_get_option('vhost_path'), TRUE , NULL,
      dt("Virtual host configuration path @path is not writable."), 
      'PROVISION_VHOST_PATH_NOT_WRITABLE');
  
  if ($writable) {
    $file = fopen(drush_get_option('vhost_path') .'/'. $file, "w");
    $text = provision_render_config($template, $data);
    fwrite($file, $text);
    fclose($file);
  }
}

/**
 * Restart Apache
 */
function _provision_apache_restart_apache($cause_error = FALSE) {
  //This is required to be configurable, due to the fact that different hosts might need to do this differently.
  //TODO : test for this instead of relying on a configuration setting?
  $return = drush_shell_exec(escapeshellcmd(drush_get_option('restart_cmd')));
  if (!$return) {
    if ($cause_error) {
      return drush_set_error('PROVISION_WEB_RESTART_FAILED', dt("Web server could not be restarted. Changes might not be available until this has been done."));
    }
    else {
      drush_log(dt("Web server could not be restarted. Changes might not be available until this has been done."), "warning");
    }
  }
  else {
    drush_log(dt('Apache has been restarted'));
  }

  return $return;
}


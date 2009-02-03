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
 * Delete virtual host file
 */
function _provision_apache_delete_vhost_config($url, $data) {
  provision_path("unlink", PROVISION_VHOST_PATH ."/$url", TRUE, dt("Removed apache virtual host configuration"));
}

/**
 * Generate virtual host file
 */
function _provision_apache_create_vhost_config($data, $url, $template = NULL) {
  if (is_null($template)) {
    $template = _provision_apache_default_template();
  }
  provision_log("Notice", dt("Generate virtual host configuration file."));
  $writable = provision_path("writable", PROVISION_VHOST_PATH, TRUE , NULL,
      dt("Virtual host configuration path @path is not writable."), 
      PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR);
  $data['extra_config'] = "# Extra configuration from modules:\n";
  foreach (drush_command_implements('provision_vhost_config') as $module) {
    $data['extra_config'] .= "# -- $module settings --\n";
    $data['extra_config'] .= module_invoke($module, 'provision_vhost_config', $url, $data) . "\n";
  }
  
  if ($writable) {
    $file = fopen(PROVISION_VHOST_PATH .'/'. $url, "w");
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
  $return = drush_shell_exec(escapeshellcmd(PROVISION_RESTART_CMD));
  if (!$return) {
    if ($cause_error) {
      provision_set_error(PROVISION_WEB_ERROR);
    }
    provision_log(($cause_error) ? "error" : "warning", "Web server could not be restarted. Changes might not be available until this has been done.");
  }
  else {
    provision_log('notice', dt('Apache has been restarted'));
  }

}


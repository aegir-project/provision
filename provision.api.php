<?php
/**
 * @file
 * Provision API
 *
 * @see drush.api.php
 * @see drush_command_invoke_all()
 */

/**
 * Possible variables to set in local.drushrc.php or another drushrc location Drush supports.
 *
 * usage:
 *   $options['provision_backup_suffix'] = '.tar.bz2';
 *
 * provision_verify_platforms_before_migrate
 *   When migrating many sites turning this off can save time, default TRUE.
 *
 * provision_backup_suffix
 *   Method to set the compression used for backups... e.g. '.tar.bz2' or '.tar.', defaults to '.tar.gz'.
 *
 * provision_apache_conf_suffix
 *   Set to TRUE to generate apache vhost files with a .conf suffix, default FALSE.
 *   This takes advantage of the IncludeOptional statment introduced in Apache 2.3.6.
 *   WARNING: After turning this on you need to re-verify all your sites, then then servers,
 *   and then cleanup the old configfiles (those without the .conf suffix).
 *
 * provision_create_local_settings_file
 *   Create a site 'local.settings.php' file if one isn't found, default TRUE.
 */

/**
 * Implements hook_drush_load(). Deprecated. Removed in Drush 7.x.
 *
 * In a drush contrib check if the frontend part (hosting_hook variant) is enabled.
 */
function hook_drush_load() {
  $features = drush_get_option('hosting_features', array());
  $hook_feature_name = 'something';

  return array_key_exists($hook_feature_name, $features) // Front-end module is installed...
    && $features[$hook_feature_name];                    // ... and enabled.
}

/**
 * Advertise what service types are available and their default
 * implementations. Services are class Provision_Service_{type}_{service} in
 * {type}/{service}/{service}_service.inc files.
 *
 * @return
 *   An associative array of type => default. Default may be NULL.
 *
 * @see provision.service.inc
 */
function hook_provision_services() {
  return array('db' => NULL);
}

/**
 * Append PHP code to Drupal's settings.php file.
 *
 * To use templating, return an include statement for the template.
 *
 * @param $uri
 *   URI for the site.
 * @param $data
 *   Associative array of data from Provision_Config_Drupal_Settings::data.
 *
 * @return
 *   Lines to add to the site's settings.php file.
 *
 * @see Provision_Config_Drupal_Settings
 */
function hook_provision_drupal_config($uri, $data) {
  return '$conf[\'reverse_proxy\'] = TRUE;';
}

/**
 * Append Apache configuration to server configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * The d() function is available to retrieve more information from the aegir
 * context.
 *
 * @param $data
 *   Associative array of data from Provision_Config_Apache_Server::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Apache_Server
 */
function hook_provision_apache_server_config($data) {
}

/**
 * Append Apache configuration to platform configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * The d() function is available to retrieve more information from the aegir
 * context.
 *
 * @param $data
 *   Associative array of data from Provision_Config_Apache_Platform::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Apache_Platform
 */
function drush_hook_provision_apache_dir_config($data) {
}

/**
 * Append Apache configuration to site vhost configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * The d() function is available to retrieve more information from the aegir
 * context.
 *
 * @param $uri
 *   URI for the site.
 * @param $data
 *   Associative array of data from Provision_Config_Apache_Site::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Apache_Site
 */
function drush_hook_provision_apache_vhost_config($uri, $data) {
}

/**
 * Append Nginx configuration to server configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * The d() function is available to retrieve more information from the aegir
 * context.
 *
 * @param $data
 *   Associative array of data from Provision_Config_Nginx_Server::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Nginx_Server
 */
function hook_provision_nginx_server_config($data) {
}

/**
 * Append Nginx configuration to platform configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * The d() function is available to retrieve more information from the aegir
 * context.
 *
 * @param $data
 *   Associative array of data from Provision_Config_Nginx_Platform::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Nginx_Platform
 */
function drush_hook_provision_nginx_dir_config($data) {
}

/**
 * Append Nginx configuration to site vhost configuration.
 *
 * To use templating, return an include statement for the template.
 *
 * The d() function is available to retrieve more information from the aegir
 * context.
 *
 * @param $uri
 *   URI for the site.
 * @param $data
 *   Associative array of data from Provision_Config_Nginx_Site::data.
 *
 * @return
 *   Lines to add to the configuration file.
 *
 * @see Provision_Config_Nginx_Site
 */
function drush_hook_provision_nginx_vhost_config($uri, $data) {
}

/**
 * Specify a different template for rendering a config file.
 *
 * @param $config
 *   The Provision_config object trying to find its template.
 *
 * @return
 *   A filename of a template to use for rendering.
 *
 * @see hook_provision_config_load_templates_alter()
 */
function hook_provision_config_load_templates($config) {
  if (is_a($config, 'Provision_Config_Drupal_Settings')) {
    $file = dirname(__FILE__) . '/custom-php-settings.tpl.php';
    return $file;
  }
}

/**
 * Alter the templates suggested for rendering a config file.
 *
 * @param $templates
 *   The array of templates suggested by other Drush commands.
 * @param $config
 *   The Provision_config object trying to find its template.
 *
 * @see hook_provision_config_load_templates()
 */
function hook_provision_config_load_templates_alter(&$templates, $config) {
  // Don't let any custom templates be used.
  $templates = array();
}

/**
 * Alter the variables used for rendering a config file.
 *
 * When implementing this hook, the function name should start with your file's name, not "drush_".
 *
 * @param $variables
 *   The variables that are about to be injected into the template.
 * @param $template
 *   The template file chosen for use.
 * @param $config
 *   The Provision_config object trying to find its template.
 *
 * @see hook_provision_config_load_templates()
 * @see hook_provision_config_load_templates_alter()
 */
function hook_provision_config_variables_alter(&$variables, $template, $config) {

  // If this is the vhost template and the http service is Docker...
  if (is_a($config, 'Provision_Config_Apache_Site') && is_a(d()->platform->service('http'), 'Provision_Service_http_apache_docker')) {

    // Force the listen port to be 80.
    $variables['http_port'] = '80';
  }
}

/**
 * Alter the array of directories to create.
 *
 * @param $mkdir
 *    The array of directories to create.
 * @param string $url
 *    The url of the site being invoked.
 */
function hook_provision_drupal_create_directories_alter(&$mkdir, $url) {
  $mkdir["sites/$url/my_special_dir"] = 02770;
  $mkdir["sites/$url/my_other_dir"] = FALSE; // Skip the chmod on this directory.
}

/**
 * Alter the array of directories to change group ownership of.
 *
 * @param $chgrp
 *    The array of directories to change group ownership of.
 * @param string $url
 *    The url of the site being invoked.
 */
function hook_provision_drupal_chgrp_directories_alter(&$chgrp, $url) {
  $chgrp["sites/$url/my_special_dir"] = d('@server_master')->web_group;
  $chgrp["sites/$url/my_other_dir"] = FALSE; // Skip the chgrp on this directory.
}

/**
 * Alter the array of directories to not to recurse into in mkdir and chgrp
 * operations.
 *
 * @param $chgrp_not_recursive
 *    The array of directories not to recurse into.
 * @param string $url
 *    The url of the site being invoked.
 */
function hook_provision_drupal_chgrp_not_recursive_directories_alter($chgrp_not_recursive, $url) {
  $chgrp_not_recursive[] = "sites/$url/my_special_dir";
  unset($chgrp_not_recursive["sites/$url"]); // Allow recursion where we otherwise wouldn't.
}

/**
 * Alter the array of directories to not to recurse into in chmod operations.
 *
 * @param $chmod_not_recursive
 *    The array of directories not to recurse into.
 * @param string $url
 *    The url of the site being invoked.
 */
function hook_provision_drupal_chmod_not_recursive_directories_alter($chmod_not_recursive, $url) {
  $chmod_not_recursive[] = "sites/$url/my_special_dir";
  unset($chmod_not_recursive["sites/$url"]); // Allow recursion where we otherwise wouldn't.
}

/**
 * Alter the settings array just before starting the provision install.
 *
 * @param $settings
 *    The array with settings.
 * @param $url
 *    The site url.
 */
function hook_provision_drupal_install_settings_alter(&$settings, $url) {
  $settings['forms']['install_configure_form']['update_status_module'] = array();
}

/**
 * Alter the options passed to 'provision-deploy' when it is invoked in
 * restore, clone and migrate tasks.
 *
 * @param array $deploy_options
 *   Options passed to the invocation of provision-deploy.
 * @param string $context
 *   The type of task invoking the hook (e.g., 'clone').
 */
function hook_provision_deploy_options_alter(&$deploy_options, $context) {
  // From hosting_s3; see: https://www.drupal.org/node/2412563
  // Inject the backup bucket name during the 'clone' task, so that it is
  // available in deploy().
  if ($bucket = drush_get_option('s3_backup_name', FALSE)) {
    $deploy_options['s3_backup_name'] = $bucket;
  }
}

/**
 * Alter the array of regexes used to filter mysqldumps.
 *
 * @param $regexes
 *   An array of patterns to match (keys) and replacement patterns (values).
 *   Setting a value to FALSE will omit the line entirely from the database
 *   dump. Defaults are set in Provision_Service_db_mysql::get_regexes().
 */
function hook_provision_mysql_regex_alter(&$regexes) {
  $regexes = array(
    // remove these lines entirely.
    '#/\*!50013 DEFINER=.*/#' => FALSE,
    // just remove the matched content.
    '#/\*!50017 DEFINER=`[^`]*`@`[^`]*`\s*\*/#' => '',
    // replace matched content as needed
    '#/\*!50001 CREATE ALGORITHM=UNDEFINED \*/#' => "/*!50001 CREATE */",
  );
}

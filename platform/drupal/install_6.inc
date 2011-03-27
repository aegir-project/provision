<?php
/**
 *  @file
 *    Rebuild all the caches
 */

$GLOBALS['url'] = d()->uri;
$GLOBALS['profile'] = d()->profile;
$GLOBALS['install_locale'] = d()->language;
$GLOBALS['base_url'] = provision_get_base_url();


require_once 'includes/install.inc';
define('MAINTENANCE_MODE', 'install');

/**
 * Verify if Drupal is installed.
 */
function install_verify_drupal() {
  $result = @db_query("SELECT name FROM {system} WHERE name = 'system'");
  return $result && db_result($result) == 'system';
}

/**
 * Verify existing settings.php
 */
function install_verify_settings() {
  global $db_prefix, $db_type, $db_url;
  // Verify existing settings (if any).
  if (!empty($db_url)) {
    // We need this because we want to run form_get_errors.

    $url = parse_url(is_array($db_url) ? $db_url['default'] : $db_url);
    $db_user = urldecode($url['user']);
    $db_pass = urldecode($url['pass']);
    $db_host = urldecode($url['host']);
    $db_port = isset($url['port']) ? urldecode($url['port']) : '';
    $db_path = ltrim(urldecode($url['path']), '/');
    $settings_file = './'. conf_path() .'/settings.php';

    return TRUE;
  }
  return FALSE;
}

function install_create_admin_user($client_email = NULL) {
  if (!$client_email) {
    $client_email = 'webmaster@localhost';
  }
  // create the admin account
  $account = user_load(1);
  $edit['name'] = 'admin';
  $edit['pass'] = user_password();
  $edit['mail'] = $client_email;
  $edit['status'] = 1;

  // temporarily disable drupal's default mail notification
  $prev = variable_get('user_mail_status_activated_notify', TRUE);
  variable_set('user_mail_status_activated_notify', FALSE);
  $account = user_save($account,  $edit);
  variable_set('user_mail_status_activated_notify', $prev);
  return $account;
}

function install_send_welcome_mail($url, $account, $profile, $language, $client_email, $onetime) {
  global $base_url;
  // Mail one time login URL and instructions.
  $from = variable_get('site_mail', ini_get('sendmail_from'));

  $mail_params['variables'] = array(
      '!username' => $account->name, '!site' => variable_get('site_name', 'Drupal'), '!login_url' => $onetime,
      '!uri' => $base_url, '!uri_brief' => preg_replace('!^https?://!', '', $base_url), '!mailto' => $account->mail, 
      '!date' => format_date(time()), '!login_uri' => url('user', array('absolute' => TRUE)), 
      '!edit_uri' => url('user/'. $account->uid .'/edit', array('absolute' => TRUE)));

  $mail_success = drupal_mail('install', 'welcome-admin', $client_email, user_preferred_language($account), $mail_params, $from, TRUE);

  if ($mail_success) {
    drush_log(t('Sent welcome mail to @client', array('@client' => $client_email)), 'message');
  }
  else {
    drush_log(t('Could not send welcome mail to @client', array('@client' => $client_email)));
  }
}

function install_mail($key, &$message, $params) {
  global $profile;
  switch ($key) {
    case 'welcome-admin':
      // allow the profile to override welcome email text
      if (file_exists("./profiles/$profile/provision_welcome_mail.inc")) {
        require_once "./profiles/$profile/provision_welcome_mail.inc";
        $custom = TRUE;
      }
      elseif (file_exists(dirname(__FILE__) . '/../provision_welcome_mail.inc')) { 
        /** use the module provided welcome email
         * We can not use drupal_get_path here,
         * as we are connected to the provisioned site's database
         */
        require_once dirname(__FILE__) . '/../provision_welcome_mail.inc';
        $custom = TRUE;
      }
      else {
        // last resort use the user-pass mail text
        $custom = FALSE;
      }

      if ($custom) {
        $message['subject'] = st($mail['subject'], $params['variables']);
        $message['body'] = st($mail['body'], $params['variables']);
      }
      else {
        $message['subject'] = _user_mail_text('pass_subject', $params['variables']);
        $message['body'] = _user_mail_text('pass_body', $params['variables']);
      }

      break;
    }
}

function install_main() {
  require_once './includes/bootstrap.inc';
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION);

  // This must go after drupal_bootstrap(), which unsets globals!
  global $profile, $install_locale, $conf, $url;

  require_once './modules/system/system.install';
  require_once './includes/file.inc';

  // Ensure correct page headers are sent (e.g. caching)
  drupal_page_header();

  // Set up $language, so t() caller functions will still work.
  drupal_init_language();

  // Load module basics (needed for hook invokes).
  include_once './includes/module.inc';
  $module_list['system']['filename'] = 'modules/system/system.module';
  $module_list['filter']['filename'] = 'modules/filter/filter.module';
  module_list(TRUE, FALSE, FALSE, $module_list);
  drupal_load('module', 'system');
  drupal_load('module', 'filter');

  // Set up theme system for the maintenance page.
  drupal_maintenance_theme();  // Check existing settings.php.

  $verify = install_verify_settings();
  // Drupal may already be installed.
  if ($verify) {
    // Since we have a database connection, we use the normal cache system.
    // This is important, as the installer calls into the Drupal system for
    // the clean URL checks, so we should maintain the cache properly.
    require_once './includes/cache.inc';
    $conf['cache_inc'] = './includes/cache.inc';

    drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_DATABASE);
    // Check if Drupal is installed.
    if (install_verify_drupal()) {
      drush_set_error('PROVISION_DRUPAL_SITE_INSTALLED', st('Site is already installed'));
      return FALSE;
    }
  }
  else {
    drush_set_error('PROVISION_CONFIG_NOT_VALID', st('Config file could not be loaded'));
    return FALSE;
  }


  drush_log(st("Installing Drupal schema"), 'install');
  // Load the profile.
  require_once "./profiles/$profile/$profile.profile";
  drush_log(st("Loading @profile install profile", array("@profile" => $profile)), 'install');

  drush_log(st("Installing translation : @locale", array("@locale" => $install_locale)), 'install');

  /**
   * Handles requirement checking
   *
   * This code is based on install_check_requirements in install.php
   * We separate this out because we want to avoid all the user interface
   * code in this function, so we only use the relevant part of it.
   */
  $requirements = drupal_check_profile($profile);
  $severity = drupal_requirements_severity($requirements);

  // If there are issues, report them.
  if ($severity == REQUIREMENT_ERROR) {
    foreach ($requirements as $requirement) {
      if (isset($requirement['severity']) && $requirement['severity'] == REQUIREMENT_ERROR) {
        drupal_set_message($requirement['description'] .' ('. st('Currently using !item !version', array('!item' => $requirement['title'], '!version' => $requirement['value'])) .')', 'error');
      }
    }
    $missing_requirement = TRUE;
  }
  if ($severity == REQUIREMENT_WARNING) {
    foreach ($requirements as $requirement) {
      if (isset($requirement['severity']) && $requirement['severity'] == REQUIREMENT_WARNING) {
        $message = $requirement['description'];
        if (isset($requirement['value']) && $requirement['value']) {
          $message .= ' ('. st('Currently using !item !version', array('!item' => $requirement['title'], '!version' => $requirement['value'])) .')';
        }
        drupal_set_message($message, 'warning');
      }
    }
  }

  if (isset($missing_requirement)) {
    return drush_set_error('PROVISION_INSTALL_MISSING_REQUIREMENTS');
  }

  // Verify existence of all required modules.
  $modules = drupal_verify_profile($profile, $install_locale);

  if (!$modules) {
    return drush_set_error('PROVISION_DRUPAL_INSTALL_FAILED');
  }

  $system_path = dirname(drupal_get_filename('module', 'system', NULL));
  require_once './'. $system_path .'/system.install';
  module_invoke('system', 'install');
  $system_versions = drupal_get_schema_versions('system');
  $system_version = $system_versions ? max($system_versions) : SCHEMA_INSTALLED;
  db_query("INSERT INTO {system} (filename, name, type, owner, status, throttle, bootstrap, schema_version) VALUES('%s', '%s', '%s', '%s', %d, %d, %d, %d)", $system_path .'/system.module', 'system', 'module', '', 1, 0, 0, $system_version);
  // Now that we've installed things properly, bootstrap the full Drupal environment

  module_rebuild_cache();
  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_FULL);
  $modules = array_diff($modules, array('system'));

  if ($install_locale != 'en') {
    $modules = array_merge($modules, array('locale'));
  }
  /**
   * Further installation tasks
   *
   * This code is based on install_tasks() in install.php
   * It has been modified to remove any calls to the user interface,
   * and run all batches in the same process, instead of in a single page
   * load.
   */

  // profile-install and profile-install-batch tasks
  $files = module_rebuild_cache();
  foreach ($modules as $module) {
    _drupal_install_module($module);
    module_enable(array($module));
    drush_log(t("Installed @module module.", 
      array("@module" => $files[$module]->info['name'])));
  }

  module_invoke_all('init');

  drush_log("Initial locale import");
  // locale-initial-import and locale-inintial-batch tasks
  if (!empty($install_locale) && ($install_locale != 'en')) {
    include_once 'includes/locale.inc';
    // Enable installation language as default site language.
    locale_add_language($install_locale, NULL, NULL, NULL, NULL, NULL, 1, TRUE);
    // Collect files to import for this language.
    $batch = locale_batch_by_language($install_locale);
    if (!empty($batch)) {
      $install_locale_batch_components = $batch['#components'];
      batch_set($batch);
      $batch =& batch_get();
      $batch['progressive'] = FALSE;
      batch_process();
    }
  }

  // configure-task
  variable_set('site_name', $url);
  variable_set('site_mail', 'webmaster@' . $url);
  variable_set('clean_url', TRUE);
  variable_set('install_time', time());

  // Get the timezone offset from system time
  $tz_offset = date('Z');
  variable_set('date_default_timezone', $tz_offset);

  menu_rebuild();

  // profile task
  // @TODO support install profiles with multiple additional tasks
  $task = 'profile';

  $function = $profile .'_profile_tasks';

  if (function_exists($function)) {
    while (!in_array($task, array('profile-finished', 'finished'))) {
      drush_log(dt("Running profile specific task : !task", array('!task' => $task)));
      module_rebuild_cache();
      module_load_all();
      // stop batch api from re-directing. ever.
      $batch =& batch_get();
      $batch['progressive'] = FALSE;
      $batch['redirect'] = null;
      $batch['operations'] = array();
      $batch['url'] = 'batch';
      batch_set($batch);

      // The profile needs to run more code, maybe even more tasks.
      // $task is sent through as a reference and may be changed!
      $output = $function($task, $url);

      $task = variable_get('install_task', $task);
      variable_del('install_task');

      // If the profile doesn't move on to a new task we assume
      // that it is done.
      if ($task == 'profile') {
        $task = 'profile-finished';
      }
    }
  }

  if ($task == 'profile-finished') {
    // profile-finished task
    // Secondary locale import 
    if (!empty($install_locale) && ($install_locale != 'en')) {
      // Collect files to import for this language. Skip components
      // already covered in the initial batch set.
      $batch = locale_batch_by_language($install_locale, NULL, $install_locale_batch_components);
      if (!empty($batch)) {
        // Start a batch, switch to 'locale-remaining-batch' task. We need to
        // set the variable here, because batch_process() redirects.
        batch_set($batch);
        $batch =& batch_get();
        $batch['progressive'] = FALSE;
        batch_process();
      }
    }
  }

  // done task
  // Rebuild menu to get content type links registered by the profile,
  // and possibly any other menu items created through the tasks.
  menu_rebuild();

  // Register actions declared by any modules.
  actions_synchronize();

  // Randomize query-strings on css/js files, to hide the fact that
  // this is a new install, not upgraded yet.
  _drupal_flush_css_js();

  cache_clear_all();
  variable_set('install_profile', $profile);

  $client_email = drush_get_option('client_email');
  $account = install_create_admin_user($client_email);
  $onetime = user_pass_reset_url($account);
  // Store the one time login link in an option so the front end can direct the user to their new site.
  drush_set_option('login_link', $onetime);
  drush_log(t('Login url: !onetime', array('!onetime' => $onetime)), 'message');

  if ($client_email) {
    install_send_welcome_mail($url, $account, $profile, $install_locale, $client_email, $onetime);
  }
  variable_set('install_task', 'done');
}
/**
 * Batch callback for batch installation of modules.
 */
function _install_module_batch($module, $module_name, &$context) {
  drush_log(dt("Enabling module : !module", array("!module" => $module)), 'success');
  _drupal_install_module($module);
  // We enable the installed module right away, so that the module will be
  // loaded by drupal_bootstrap in subsequent batch requests, and other
  // modules possibly depending on it can safely perform their installation
  // steps.
  module_enable(array($module));
  $context['results'][] = $module;
  $context['message'] = st('Installed %module module.', array('%module' => $module_name));
}
install_main();

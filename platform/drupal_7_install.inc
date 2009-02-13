<?php
/**
 *  @file
 *    Rebuild all the caches
 */

$GLOBALS['url'] = $data['site_url'];
$GLOBALS['profile'] = $data['profile'];
$GLOBALS['install_locale'] = $data['language'];
$GLOBALS['client_email'] = $data['client_email'];

require_once 'includes/install.inc';
define('MAINTENANCE_MODE', 'install');

/**
 * Verify if Drupal is installed.
 */
function install_verify_drupal() {
  try { 
    $result = @db_query("SELECT name FROM {system} WHERE name = 'system'");
    return $result && db_result($result) == 'system';
  }
  catch (Exception $e) {
  }

}

/**
 * Verify existing settings.php
 */
function install_verify_settings() {
  global $db_prefix, $db_type, $db_url, $databases;
  return TRUE;
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

function install_send_welcome_mail($url, $profile, $language, $client_email) {
  if ($client_email) {
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

    // Mail one time login URL and instructions.
    $from = variable_get('site_mail', ini_get('sendmail_from'));
    $onetime = user_pass_reset_url($account);
    $mail_params['variables'] = array(
      '!username' => $account->name, '!site' => variable_get('site_name', 'Drupal'), '!login_url' => $onetime,
      '!uri' => $base_url, '!uri_brief' => preg_replace('!^https?://!', '', $base_url), '!mailto' => $account->mail, 
      '!date' => format_date(time()), '!login_uri' => url('user', array('absolute' => TRUE)), 
      '!edit_uri' => url('user/'. $account->uid .'/edit', array('absolute' => TRUE)));

    $mail_success = drupal_mail('install', 'welcome-admin', $account->mail, user_preferred_language($account), $mail_params, $from, TRUE);

    if ($mail_success) {
      provision_log('message', t('Sent welcome mail to @client', array('@client' => $client_email)));
    }
    else {
      provision_log('notice', t('Could not send welcome mail to @client', array('@client' => $client_email)));
    }
    provision_log('message', t('Login url: !onetime', array('!onetime' => $onetime)));
  }
}

function install_mail($key, &$message, $params) {
  global $profile, $install_locale, $client_email, $conf, $url, $base_url;
  switch ($key) {
    case 'welcome-admin':
      // allow the profile to override welcome email text
      if (file_exists("./profiles/$profile/provision_welcome_mail.inc")) {
        require_once "./profiles/$profile/provision_welcome_mail.inc";
        $custom = TRUE;
      }
      elseif (file_exists(dirname(__FILE__) . '/provision_welcome_mail.inc')) { 
        /** use the module provided welcome email
         * We can not use drupal_get_path here,
         * as we are connected to the provisioned site's database
         */
        require_once dirname(__FILE__) . '/provision_welcome_mail.inc';
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
  drupal_bootstrap(DRUPAL_BOOTSTRAP_CONFIGURATION);

  // This must go after drupal_bootstrap(), which unsets globals!
  global $profile, $install_locale, $client_email, $conf, $url;

  require_once './modules/system/system.install';
  require_once './includes/file.inc';

  // Ensure correct page headers are sent (e.g. caching)
  drupal_page_header();

  // Set up $language, so t() caller functions will still work.
  drupal_init_language();

  // Load module basics (needed for hook invokes).
  include_once './includes/module.inc';
  include_once './includes/session.inc';
  $module_list['system']['filename'] = 'modules/system/system.module';
  $module_list['filter']['filename'] = 'modules/filter/filter.module';
  module_list(TRUE, FALSE,  $module_list);
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
    require_once DRUPAL_ROOT . '/includes/cache.inc';
    $conf['cache_inc'] = 'includes/cache.inc';

    // Initialize the database system. Note that the connection
    // won't be initialized until it is actually requested.
    require_once DRUPAL_ROOT . '/includes/database/database.inc';

    // Check if Drupal is installed.
    if (install_verify_drupal()) {
      provision_set_error(PROVISION_SITE_INSTALLED);
      provision_log('error', st('Site is already installed'));
      return FALSE;
    }
  }
  else {
    // Since no persistent storage is available yet, and functions that check
    // for cached data will fail, we temporarily replace the normal cache
    // system with a stubbed-out version that short-circuits the actual
    // caching process and avoids any errors.
    require_once DRUPAL_ROOT . '/includes/cache-install.inc';
    $conf['cache_inc'] = 'includes/cache-install.inc';

  }


  provision_log("install", st("Installing Drupal schema"));
  // Load the profile.
  require_once "./profiles/$profile/$profile.profile";
  provision_log("install", st("Loading @profile install profile", array("@profile" => $profile)));

  provision_log("install", st("Installing translation : @locale", array("@locale" => $install_locale)));

  /**
   * Handles requirement checking
   *
   * This code is based on install_check_requirements in install.php
   * We separate this out because we want to avoid all the user interface
   * code in this function, so we only use the relevant part of it.
   */
  $missing_requirement = FALSE;
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

  if ($missing_requirement) {
    provision_set_error(PROVISION_INSTALL_ERROR);
    provision_log('error', t("Could not meet the requirements for installing the drupal profile"));
    return FALSE;
  }

  // Verify existence of all required modules.
  $requirements = drupal_verify_profile($profile, $install_locale);
  // Check the severity of the requirements reported.
  $severity = drupal_requirements_severity($requirements);

  if ($severity == REQUIREMENT_ERROR) {
    provision_log('error', t('Missing requirements'));
    provision_set_error(PROVISION_FRAMEWORK_ERROR);
    return FALSE;
  }

  drupal_install_system();
  $modules = array_diff(drupal_get_profile_modules($profile, $install_locale), array('system'));

  drupal_install_init_database(); 
  drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

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
    provision_log("notice", t("Installed @module module.", 
      array("@module" => $files[$module]->info['name'])));
  }
  module_load_all();
  module_invoke_all('init');

  // locale-initial-import and locale-inintial-batch tasks
  if (!empty($install_locale) && ($install_locale != 'en')) {
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

  menu_rebuild();

  // profile task
  // @TODO support install profiles with multiple additional tasks
  $task = 'profile';
  $function = $profile .'_profile_tasks';
  if (function_exists($function)) {
    // The profile needs to run more code, maybe even more tasks.
    // $task is sent through as a reference and may be changed!
    $output = $function($task, $url);
  }

  // profile-finished task
  // Secondary locale import 
  if (!empty($install_locale) && ($install_locale != 'en')) {
    include_once DRUPAL_ROOT . '/includes/locale.inc';
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

  if ($client_email) {
    install_send_welcome_mail($url, $profile, $install_locale, $client_email);
  }
}
install_main();

<?php
/**
 *  @file
 *    Rebuild all the caches
 */

require_once(dirname(__FILE__) . '/../provision.inc');
if (sizeof($argv) == 5) {
  // Fake the necessary HTTP headers that Drupal needs:
  provision_external_init($argv[1], FALSE);
  $GLOBALS['profile'] = $argv[2];
  $GLOBALS['install_locale'] = $argv[3];
  $GLOBALS['client_email'] = $argv[4];

  require_once './includes/install.inc';
}
else {
  provision_set_error(PROVISION_FRAMEWORK_ERROR);
  provision_log("error", "USAGE: install.php url profile locale email\n");
}

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
  if ($db_url != 'mysql://username:password@localhost/databasename') {
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
  // create the admin account or change some parameters if the install profile
  // already created one
  $account = user_load(array('uid' => 1));
  if (!$account) {
    $account = new stdClass();
  }
  $edit['name'] = 'admin';
  $edit['pass'] = user_password();
  $edit['mail'] = $client_email;
  $edit['status'] = 1;
  $account = user_save($account,  $edit);

  // Mail one time login URL and instructions.
  $from = variable_get('site_mail', ini_get('sendmail_from'));
  $onetime = user_pass_reset_url($account);
  $variables = array(
    '!username' => $account->name, '!site' => variable_get('site_name', 'Drupal'), '!login_url' => $onetime,
    '!uri' => $base_url, '!uri_brief' => preg_replace('!^https?://!', '', $base_url), '!mailto' => $account->mail, 
    '!date' => format_date(time()), '!login_uri' => url('user', NULL, NULL, TRUE), 
    '!edit_uri' => url('user/'. $account->uid .'/edit', NULL, NULL, TRUE));

  // allow the profile to override welcome email text
  if (file_exists("./profiles/$profile/provision_welcome_mail.inc")) {
    require_once "./profiles/$profile/provision_welcome_mail.inc";
    $mailkey = 'welcome-mail-admin';
  }
  elseif (file_exists(dirname(__FILE__) . '/provision_welcome_mail.inc')) { 
    /** use the module provided welcome email
     * We can not use drupal_get_path here,
     * as we are connected to the provisioned site's database
     */
    require_once dirname(__FILE__) . '/provision_welcome_mail.inc';
    $mailkey = 'welcome-mail-admin';
  }
  else {
    // last resort use the user-pass mail text
    $mailkey = 'user-pass';
  }

  if ($mailkey == 'welcome-mail-admin') {
    $subject = st($mail['subject'], $variables);
    $body = st($mail['body'], $variables);
  }
  else {
    $subject = _user_mail_text('pass_subject', $variables);
    $body = _user_mail_text('pass_body', $variables);
  }

  $mail_success = drupal_mail($mailkey, $account->mail, $subject, $body, $from);

  if ($mail_success) {
    provision_log('message', t('Sent welcome mail to @client', array('@client' => $client_email)));
  }
  else {
    provision_log('notice', t('Could not send welcome mail to @client', array('@client' => $client_email)));
  }
  provision_log('message', t('Login url: !onetime', array('!onetime' => $onetime)));

}

function install_main() {
  require_once './includes/bootstrap.inc';
  drupal_bootstrap(DRUPAL_BOOTSTRAP_CONFIGURATION);
  // This must go after drupal_bootstrap(), which unsets globals!
  global $profile, $install_locale, $client_email;
  require_once './modules/system/system.install';
  require_once './includes/file.inc';

  // Check existing settings.php.
  $verify = install_verify_settings();
  // Drupal may already be installed.
  if ($verify) {
    // Establish a connection to the database.
    require_once './includes/database.inc';
    db_set_active();
    // Check if Drupal is installed.
    if (install_verify_drupal()) {
      provision_set_error(PROVISION_SITE_INSTALLED);
      provision_log('error', st('Site is already installed'));
      return FALSE;
    }
  }
  else {
    provision_set_error(PROVISION_FRAMEWORK_ERROR);
    provision_log('error', st('Config file could not be loaded'));
    return FALSE;
  }
  // Load module basics (needed for hook invokes).
  include_once './includes/module.inc';
  $module_list['system']['filename'] = 'modules/system/system.module';
  $module_list['filter']['filename'] = 'modules/filter/filter.module';
  module_list(TRUE, FALSE, FALSE, $module_list);
  drupal_load('module', 'system');
  drupal_load('module', 'filter');


  provision_log("install", st("Installing Drupal schema"));
  // Load the profile.
  require_once "./profiles/$profile/$profile.profile";
  provision_log("install", st("Loading @profile install profile", array("@profile" => $profile)));
  $requirements = drupal_check_profile($profile);
  $severity = drupal_requirements_severity($requirements);

  // If there are issues, report them.
  if ($severity == REQUIREMENT_ERROR) {
    foreach ($requirements as $requirement) {
      if (isset($requirement['severity']) && $requirement['severity'] == REQUIREMENT_ERROR) {
        drupal_set_message($requirement['descristion'] .' ('. st('Currently using !item !version', array('!item' => $requirement['title'], '!version' => $requirement['value'])) .')', 'error');
      }
    }

    return FALSE;
  }

  // Verify existence of all required modules.
  $modules = drupal_verify_profile($profile, $install_locale);

  if (!$modules) {
    provision_set_error(PROVISION_FRAMEWORK_ERROR);
    return FALSE;
  }
  foreach ($modules as $module) {
    provision_log("success", st("Installing module : @module", array("@module" => $module)));
  }
  // Perform actual installation defined in the profile.
  drupal_install_profile($profile, $modules);

  drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);

  // Show profile finalization info.
  $function = $profile .'_profile_final';
  if (function_exists($function)) {
    // More steps required
    $profile_message = $function();
  }

  variable_set('install_profile', $profile);
  if ($client_email) {
    install_send_welcome_mail($url, $profile, $language, $client_email);
  }
}
$data = array();
install_main($url, $data);
provision_output($argv[1], $data);

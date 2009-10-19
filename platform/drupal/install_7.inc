<?php
/**
 *  @file
 *    Rebuild all the caches
 */

$GLOBALS['url'] = drush_get_option('site_url');
$GLOBALS['profile'] = drush_get_option('profile');
$GLOBALS['install_locale'] = drush_get_option('language');
$GLOBALS['client_email'] = drush_get_option('client_email');

define('MAINTENANCE_MODE', 'install');

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
      drush_log(t('Sent welcome mail to @client', array('@client' => $client_email)), 'message');
    }
    else {
      drush_log(t('Could not send welcome mail to @client', array('@client' => $client_email)));
    }
    drush_log(t('Login url: !onetime', array('!onetime' => $onetime)), 'message');
  }
}

function install_main() {
  global $profile, $install_locale, $client_email, $conf, $url, $base_url;

  require_once('install.php');

  $account_pass = provision_password();
  $settings = array(
    'parameters' => array(
      'profile' => $profile,
      'locale' => $install_locale,
    ),
    'forms' => array(
   /*   'install_settings_form' =>  array(
          'driver' => drush_get_option('db_type'),
          'host' => drush_get_option('db_host'),
          'port' => drush_get_option('db_port'),
          'database' => drush_get_option('db_name'),
          'username' => drush_get_option('db_user'),
          'password' => drush_get_option('db_passwd'),
        ),*/
      'install_configure_form' => array(
        'site_name' => $url,
        'site_mail' => $client_email ? $client_email : 'admin@example.com',
        'theme' => 'garland',
        'account' => array(
          'name' => 'admin',
          'mail' => $client_email ? $client_email : 'admin@example.com',
          'pass' => array(
            'pass1' => $account_pass,
            'pass2' => $account_pass,
          ),
        ),
        'update_status_module' => array(
          1 => TRUE,
          2 => TRUE,
        ),
        'clean_url' => drush_get_option('clean_url', TRUE),
      ),
    ),
  );
  set_exception_handler('install_exception_handler');
  set_error_handler('install_exception_handler');
  try {
    install_drupal($settings);
  }
  catch (Exception $e) {
    drush_log($e->message());
    dlm($e);
  }
  _provision_drupal_create_directories($url);

#  if ($client_email) {
#    install_send_welcome_mail($url, $profile, $install_locale, $client_email);
#  }
}
install_main();

function install_exception_handler() {
  dlm(func_get_args());
}

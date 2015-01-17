<?php
/**
 *  @file
 *    Rebuild all the caches
 */

use Drupal\Core\Url;

$GLOBALS['url'] = d()->uri;
$GLOBALS['profile'] = d()->profile;
$GLOBALS['install_locale'] = d()->language;
$GLOBALS['base_url'] = provision_get_base_url();

define('MAINTENANCE_MODE', 'install');

function install_send_welcome_mail($url, $account, $language, $client_email, $onetime) {
  global $base_url;
  if ($client_email) {
    // Mail one time login URL and instructions.
    $from = \Drupal::config('system.site')->get('mail');

    $username = $account->getUsername();
    $site = \Drupal::config('system.site')->get('name');
    $uri_brief = preg_replace('!^https?://!', '', $base_url);
    $edit_uri = $base_url . '/user/1/edit';
    $mailto = $account->getEmail();
    $date = format_date(time());

    $mail_params['variables'] = array(
      '!username' => $username,
      '!site' => $site,
      '!login_url' => $onetime,
      '!uri' => $base_url,
      '!uri_brief' => $uri_brief,
      '!edit_uri' => $edit_uri,
      '!mailto' => $mailto,
      '!date' => $date,
    );

    $langcode = $account->getPreferredLangcode();

    $mail_success = \Drupal::service('plugin.manager.mail')->mail('install', 'welcome-admin', $mailto, $langcode, $mail_params, $from, TRUE);

    if ($mail_success) {
      drush_log(dt('Sent welcome mail to @client', array('@client' => $client_email)), 'message');
    }
    else {
      drush_log(dt('Could not send welcome mail to @client', array('@client' => $client_email)));
    }
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
        $message['subject'] = t($mail['subject'], $params['variables']);
        $message['body'][] = t($mail['body'], $params['variables']);
      }
      else {
        $message['subject'] = _user_mail_text('pass_subject', $params['variables']);
        $message['body'][] = _user_mail_text('pass_body', $params['variables']);
      }

      break;
    }
}


function install_main() {
  global $profile, $install_locale, $conf, $url, $base_url;

  $client_email = drush_get_option('client_email');

  require_once DRUPAL_ROOT . '/core/includes/install.core.inc';

  drush_bootstrap(DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION);

  // We have an existing settings.php.
  require_once DRUSH_BASE_PATH . '/commands/sql/sql.drush.inc';

  $sql = drush_sql_get_class();
  $db_spec = $sql->db_spec();

  $db_spec['db_prefix'] = $GLOBALS['db_prefix'];

  if ($db_spec['driver'] == 'mysqli') {
    $db_spec['driver'] = 'mysql';
  }


  unset($config['site_name']);
  unset($config['site_mail']);
  unset($GLOBALS['db_url']);

  $account_pass = provision_password();
  $settings = array(
    'parameters' => array(
      'profile' => $profile,
      'locale' => $install_locale,
    ),
    'settings_verified' => TRUE,
    'forms' => array(
      'install_settings_form' => $db_spec,
      'install_configure_form' => array(
        'site_name' => $url,
        'site_mail' => $client_email ? $client_email : 'admin@example.com',
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
  try {
    install_drupal($settings);
  }
  catch (Exception $e) {
    drush_set_error('DRUPAL_INSTALL_FAILED');
    dlm($e);
  }
  _provision_drupal_create_directories();

  // Set files paths
  \Drupal::configFactory()->getEditable('system.file')
    ->set('path.private', "sites/$url/private/files")
    ->set('path.temporary', "sites/$url/private/temp")
    ->save();

  $account = user_load(1);

  $onetime = user_pass_reset_url($account);
  // Store the one time login link in an option so the front end can direct the user to their new site.
  drush_set_option('login_link', $onetime);
  drush_log(dt('Login url: !onetime', array('!onetime' => $onetime)), 'message');

  if ($client_email) {
    install_send_welcome_mail($url, $account, $install_locale, $client_email, $onetime);
  }
}
install_main();

function install_exception_handler() {
  dlm(func_get_args());
}

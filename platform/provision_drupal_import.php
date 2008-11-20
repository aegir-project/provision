<?php
if ($argv[1]) {
  $_SERVER['HTTP_HOST'] = $argv[1];
  $_SERVER['SCRIPT_NAME'] = '/index.php';
  $command_line = true;
  require_once('includes/bootstrap.inc');
  drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
  require_once(dirname(__FILE__) . '/../provision.inc');
}
else {
  print "USAGE: provision_drupal_import.php url\n";
  exit(PROVISION_FRAMEWORK_ERROR);
}

if ($parts = @parse_url($GLOBALS['db_url'])) {
  $data['db_type'] = $parts['scheme'];
  $data['db_user'] = $parts['user'];
  $data['db_host'] = $parts['host'];
  $data['db_passwd'] = $parts['pass'];
  $data['db_name'] = substr($parts['path'], 1);

  $data['profile'] = variable_get('install_profile', 'default');
  $language = variable_get('language_default',
    (object) array('language' => 'en', 'name' => 'English', 'native' => 'English', 
                   'direction' => 0, 'enabled' => 1, 'plurals' => 0, 'formula' => '',
                   'domain' => '', 'prefix' => '', 'weight' => 0, 'javascript' => ''));
  $data['language'] = $language->language;
}
provision_output($argv[1], $data);

print(serialize($data));
exit(PROVISION_SUCCESS);

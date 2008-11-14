<?php
include_once('provision_drupal_bootstrap.inc');

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
print(serialize($data));
exit(1);

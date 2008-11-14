<?php
include_once('provision_drupal_bootstrap.inc');

if ($parts = @parse_url($GLOBALS['db_url'])) {
  $data['db_type'] = $parts['scheme'];
  $data['db_user'] = $parts['user'];
  $data['db_host'] = $parts['host'];
  $data['db_passwd'] = $parts['pass'];
  $data['db_name'] = substr($parts['path'], 1);


  $data['profile'] = variable_get('install_profile', 'default');
  $has_locale = db_result(db_query("SELECT status FROM {system} WHERE type='module' AND name='locale'"));
  if ($has_locale) {
    $locale = db_result(db_query("SELECT locale FROM {locales_meta} WHERE isdefault=1 AND enabled=1"));
  }
  $data['language'] = ($locale) ? ($locale) : 'en';
}
print(serialize($data));
exit(1);

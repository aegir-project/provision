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
  $has_locale = db_result(db_query("SELECT status FROM {system} WHERE type='module' AND name='locale'"));
  if ($has_locale) {
    $locale = db_result(db_query("SELECT locale FROM {locales_meta} WHERE isdefault=1 AND enabled=1"));
  }
  $data['language'] = ($locale) ? ($locale) : 'en';
}
provision_output($argv[1], $data);

print(serialize($data));
exit(PROVISION_SUCCESS);

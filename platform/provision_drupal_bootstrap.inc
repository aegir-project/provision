<?php
if ($argv[1]) {
  $_SERVER['HTTP_HOST'] = $argv[1];
  $_SERVER['SCRIPT_NAME'] = '/index.php';
  $command_line = true;
  require_once('includes/bootstrap.inc');
  drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
}
else {
  print "USAGE: provision_drupal_import.php url\n";
  exit(32);
}


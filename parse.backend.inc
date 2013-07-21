<?php

function drush_provision_backend_parse() {
  return drush_backend_parse_output(file_get_contents('php://stdin'), array('integrate' => TRUE, 'log' => TRUE, 'output' => TRUE), FALSE);
}

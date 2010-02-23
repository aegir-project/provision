<?php

/**
 */
function drush_provision_server() {
  _provision_create_dir(drush_get_option('config_path'), dt('Provision configuration'), 0711);
  _provision_create_dir(drush_get_option('config_path') . '/includes', dt('Provision PHP configuration'), 0711);
}

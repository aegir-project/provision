<?php

// $Id$

/**
 * @file
 *   Provision hooks for the install command
 */

/**
 * Implementation of hook_provision_post_install
 */
function drush_provision_apache_post_provision_install($url = NULL) {
  _provision_apache_create_vhost_config($url);
  _provision_apache_restart_apache();  
}


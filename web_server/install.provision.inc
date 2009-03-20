<?php

// $Id$

/**
 * @file
 *   Provision hooks for the install command
 */

/**
 * Implementation of hook_provision_pre_install
 */
function provision_apache_provision_pre_install($url = NULL) {
  return _provision_apache_create_vhost_config($url);
}

/**
 * Implementation of hook_provision_post_install
 */
function provision_apache_provision_post_install($url = NULL) {
  return _provision_apache_restart_apache();  
}


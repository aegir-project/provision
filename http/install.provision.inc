<?php

// $Id$

/**
 * @file
 *   Provision hooks for the install command
 */

/**
 * Implementation of hook_provision_post_install
 */
function drush_http_post_provision_install() {
  provision_service('http')->create_site_config();
  provision_service('http')->parse_configs();
}


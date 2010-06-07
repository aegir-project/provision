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
  d()->service('http')->create_site_config();
  d()->service('http')->parse_configs();
}


<?php

// $Id: verify.provision.inc,v 1.5 2009/03/26 01:40:39 anarcat Exp $

/**
 * @file
 *   Provision hooks for the verify command
 **/

/**
 * Implementation of hook_provision_verify
 *
 * Can't be rolled back.
 */
function drush_dns_provision_verify() {
  switch (d()->type) {
    case 'site':
      d()->service('dns')->create_config('zone');
      break;
  }
}

function drush_dns_post_provision_verify() {
  if (d()->type == 'site') {
    d()->service('dns')->create_config('host');
    d()->service('dns')->parse_configs();
  }
}

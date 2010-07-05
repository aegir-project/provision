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
  case 'server':
    // we are verifying a server
// XXX: is this still necessary now that we have verify stuff in the service config?
//     _provision_create_dir(PROVISION_NAMED_PATH, dt('Provision DNS configuration'), 0755);
// # The above perms should really be 750, but that means the aegir user must be in the 'bind' group, so this will work:
//     provision_path("chgrp", PROVISION_NAMED_PATH, 'bind',
// 		   dt('Changed group ownership of '.PROVISION_NAMED_PATH.' to @confirm'),
//                    dt('Could not change group ownership of '.PROVISION_NAMED_PATH.' to @confirm'));
    break;
  case 'site':
    d()->service('dns')->create_host(d()->uri);
    break;
  }
}

function drush_dns_post_provision_verify() {
  if (d()->type == 'site') {
    d()->service('dns')->commit();
  }
}

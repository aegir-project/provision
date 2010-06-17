<?php

// $Id: verify.provision.inc,v 1.5 2009/03/26 01:40:39 anarcat Exp $

/**
 * @file
 *   Provision hooks for the verify command
 **/

include_once("dns.api.inc");

/**
 * Implementation of hook_provision_verify
 *
 * Can't be rolled back.
 */
function drush_dns_provision_verify() {
  switch (d()->type) {
  case 'server':
    // we are verifying a server
# Where does this constant come from now?
//     _provision_create_dir(PROVISION_NAMED_PATH, dt('Provision DNS configuration'), 0755);
// # The above perms should really be 750, but that means the aegir user must be in the 'bind' group, so this will work:
//     provision_path("chgrp", PROVISION_NAMED_PATH, 'bind',
// 		   dt('Changed group ownership of '.PROVISION_NAMED_PATH.' to @confirm'),
//                    dt('Could not change group ownership of '.PROVISION_NAMED_PATH.' to @confirm'));
    break;
  case 'site':
    drush_log("notice", "Hit dns_provision_verify hook: $url");
    $url_parts = _dns_split_url($url);
    if (!isset($url_parts['zone'])) {
      drush_set_error('PROVISION_FRAMEWORK_ERROR');
      return;
    }

    $zone = dns_create_zone($url_parts['zone']);   # initializes the zone for the domain (if it doesn't exist)
    dns_create_record($url_parts['host'], $zone);    # creates the RR for the (sub)domain
    dns_commit(drush_get_option('dns_id'));                            # makes the changes live (ie: restart bind)
    drush_log(dt("Edited zone file %zone", array("%zone" => $zone)));
    break;
  }
}

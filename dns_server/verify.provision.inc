<?php

// $Id$

/**
 * @file
 *   Provision hooks for the verify command
 **/

/**
 * Implementation of hook_provision_verify
 *
 * Can't be rolled back.
 */
function provision_dns_provision_verify(&$data, $url = NULL) {
  if (!$url) {
    // we are verifying a platform
# Where does this constant come from now?
#    _provision_create_dir(PROVISION_NAMED_PATH, t('Provision DNS configuration'), 0755);
# The above perms should really be 750, but that means the aegir user must be in the 'bind' group, so this will work:
#    provision_path("chgrp", PROVISION_NAMED_PATH, 'bind', 
#		   t('Changed group ownership of '.PROVISION_NAMED_PATH.' to @confirm'),
#                   t('Could not change group ownership of '.PROVISION_NAMED_PATH.' to @confirm'));
  } else {
    drush_log("notice", "Hit provision_dns_provision_verify hook: $url");
#    $url_parts = _provision_dns_split_url($url);
    if (!isset($url_parts['zone'])) { 
      drush_set_error(PROVISION_FRAMEWORK_ERROR);
      return;
    }

#    $zone = provision_dns_create_zone($url_parts['zone'], $data);   # initializes the zone for the domain (if it doesn't exist)
#    provision_dns_create_record($url_parts['host'], $zone, $data);    # creates the RR for the (sub)domain
#    provision_dns_commit($data['dns_id']);                            # makes the changes live (ie: restart bind)
  }
}


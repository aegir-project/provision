<?php

// $Id$

/**
 * @file
 *   Provision hooks for the install command
 **/

/**
 * Implementation of hook_provision_pre_install
 **/
function provision_dns_provision_pre_install($url = NULL) {
  # data should now contain zone info directly, rather than having to pull them from 
  if ($url) {
#    $url_parts = _provision_dns_split_url($url);
#    if (!isset($url_parts['zone'])) { 
#      drush_set_error(PROVISION_FRAMEWORK_ERROR);
#      return;
#    }

    # the zone object should be passed in..
#    $zone = provision_dns_create_zone($url_parts['zone']);   # initialize zone for this domain (if it doesn't exist)
#    provision_dns_create_record($url_parts['host'], $zone);    # creates the RR for the (sub)domain
#    provision_dns_commit(drush_get_option('dns_id'));                            # makes the changes live (ie: restart bind)
  }
}

/**
 * Implementation of hook_provision_pre_install_rollback
 **/
function provision_dns_provision_pre_install_rollback($url = NULL) {
  if ($url) {
#    provision_dns_delete($url);
  }
}

/**
 * Implementation of hook_provision_post_install
 **/
function provision_dns_provision_post_install($url = NULL) {
  # commit and restart bind
}

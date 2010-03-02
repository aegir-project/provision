<?php

// $Id: install.provision.inc,v 1.7 2009/05/07 22:04:30 adrian Exp $

/**
 * @file
 *   Provision hooks for the install command
 **/

function drush_provision_dns_pre_provision_install($url = NULL) {
  # data should now contain zone info directly, rather than having to pull them from
  if ($url) {
#    $url_parts = _provision_dns_split_url($url);
#    if (!isset($url_parts['zone'])) {
#      drush_set_error('PROVISION_FRAMEWORK_ERROR');
#      return;
#    }

    # the zone object should be passed in..
#    $zone = provision_dns_create_zone($url_parts['zone']);   # initialize zone for this domain (if it doesn't exist)
#    provision_dns_create_record($url_parts['host'], $zone);    # creates the RR for the (sub)domain
#    provision_dns_commit(drush_get_option('dns_id'));                            # makes the changes live (ie: restart bind)
  }
}

function drush_provision_dns_pre_provision_install_rollback($url = NULL) {
  if ($url) {
#    provision_dns_delete($url);
  }
}

function drush_provision_dns_post_provision_install($url = NULL) {
  # commit and restart bind
}

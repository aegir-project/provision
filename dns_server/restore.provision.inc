<?php

// $Id$

/**
 * @file
 *   Provision hooks for the restore command
 **/

/**
 * Implementation of hook_provision_pre_restore
 * This is a duplicate of the _pre_install hook
 **/
function provision_dns_pre_provision_restore($url = NULL) {
  # re-install the zonefiles and/or resource records for the site?
#  _provision_dns_provision_pre_restore($url, $data);
}

/**
 * Implementation of hook_provision_pre_restore_rollback
 * This duplicates the _pre_install_rollback hook
 **/
function provision_dns_pre_provision_restore_rollback($url = NULL) {
  # remove them again?
  if ($url) {
#    _provision_dns_delete($url);
  }
}

function provision_dns_post_provision_restore($url = NULL) {
  # restart bind?
}

function provision_dns_post_provision_restore_rollback($url = NULL) {
  # restart bind
}

<?php

// $Id: restore.provision.inc,v 1.4 2009/05/07 22:04:30 adrian Exp $

/**
 * @file
 *   Provision hooks for the restore command
 **/

/**
 * Implementation of hook_provision_pre_restore
 * This is a duplicate of the _pre_install hook
 **/
function drush_dns_pre_provision_restore($url = NULL) {
  # re-install the zonefiles and/or resource records for the site?
#  _dns_provision_pre_restore($url, $data);
}

/**
 * Implementation of hook_provision_pre_restore_rollback
 * This duplicates the _pre_install_rollback hook
 **/
function drush_dns_pre_provision_restore_rollback($url = NULL) {
  # remove them again?
  if ($url) {
#    _dns_delete($url);
  }
}

function drush_dns_post_provision_restore($url = NULL) {
  # restart bind?
}

function drush_dns_post_provision_restore_rollback($url = NULL) {
  # restart bind
}

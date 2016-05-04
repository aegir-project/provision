<?php
/**
 * @file Filesystem access module.
 *
 * Handle writing and syncing configuation files across multiple servers.
 * Provides an interface to common path handling operations, through the path
 * helper method, which will take care of verification and any error logging
 * required.
 */
include_once('provision.inc');

function provision_file() {
  static $instance = NULL;

  if (is_null($instance)) {
    $instance = new Provision_FileSystem();
  }

  return $instance;
}

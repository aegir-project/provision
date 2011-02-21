<?php
/**
 * @file
 *   Provision hooks for the provision backup_delete command.
 */

/**
 * Provision backup_delete command
 *
 * These are the hooks that will be executed by the drush_invoke function
 * when deleting a backup file from the backend.
 */

function drush_provision_drupal_provision_backup_delete($backup = null) {
  if($backup) {
    $backups = explode(',', $backup);
    foreach ($backups as $filename) {
      $success =  provision_file()->unlink($filename)
       ->succeed('Removed backup file @path')
       ->fail('Could not remove backup file @path');
    }
  }
  else {
    drush_set_error(dt('No backup files provided'));
  }
}

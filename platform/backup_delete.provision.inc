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

/**
 * Implementation of drush_hook_post_provision_backup_delete()
 *
 * If we have a client name and backups provided, cleanup the backup
 * symlinks created for that client.
 *
 * Will not provoke a rollback if there is no symlink, for backward
 * compatibility.
 */
function drush_provision_drupal_post_provision_backup_delete($backup = null) {
  if ($backup && d()->client_name) {
    $backup_dir = d()->server->clients_path . '/' . d()->client_name . '/backups';
    $backups = explode(',', $backup);
    foreach ($backups as $filename) {
      provision_file()->unlink($backup_dir . '/' . basename($filename))
        ->succeed('Deleted backup symlink @path');
    }
  }
}

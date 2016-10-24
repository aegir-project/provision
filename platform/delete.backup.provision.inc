<?php
/**
 * @file
 * Provision hooks for the provision backup-delete command.
 */

/**
 * Provision backup-delete command.
 *
 * These are the hooks that will be executed by the drush_invoke function
 * when deleting a backup file from the backend.
 *
 * @param string $backup
 *   A comma-separated list of backup filenames.
 */
function drush_provision_drupal_provision_backup_delete($backup) {
  $backups = explode(',', $backup);
  foreach ($backups as $filename) {
    $success = provision_file()->unlink($filename)
      ->succeed('Removed backup file @path')
      ->fail('Could not remove backup file @path');
  }
}

/**
 * Implements drush_hook_post_COMMAND().
 *
 * If we have a client name, cleanup the backup symlinks created for that
 * client.
 *
 * Will not provoke a rollback if there is no symlink, for backward
 * compatibility.
 *
 * @param string $backup
 *   A comma-separated list of backup filenames.
 */
function drush_provision_drupal_post_provision_backup_delete($backup) {
  if (d()->client_name) {
    $backup_dir = d()->server->clients_path . '/' . d()->client_name . '/backups';
    $backups = explode(',', $backup);
    foreach ($backups as $filename) {
      provision_file()->unlink($backup_dir . '/' . basename($filename))
        ->succeed('Deleted backup symlink @path');
    }
  }
}

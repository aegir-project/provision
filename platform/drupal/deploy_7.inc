<?php

$new_url = drush_get_option('uri');
$old_url = drush_get_option('old_uri');

/**
 * @file
 *   Handle site migration tasks for redeployed sites.
 *   This is primarily to handle the rename of the sites
 *   directories.
 */

if ($new_url != $old_url) {
  drush_log(
    dt('Changed paths from sites/@old_url to sites/@new_url',
    array('@old_url' => $old_url, '@new_url' => $new_url)));
}


<?php

// Fetch the cron key from the Drupal site and set in a drush context.

drush_set_option('cron_key', variable_get('cron_key', 'drupal'));
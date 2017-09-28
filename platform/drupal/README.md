# provision/platform/drupal/*.inc

All files in this folder are included directly into the Drupal site codebase the task is being run on.

The code in these files is run fully bootstrapped to your Drupal sites.

This means the code must be compatible with the Drupal version of the site.

This came up as an issue when Drupal 8.4 moved to Symfony 3: The Yaml::parse() method changed, so we have to change our code to reflect that.

See https://www.drupal.org/node/2911855 for more details.
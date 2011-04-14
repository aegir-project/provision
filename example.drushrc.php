/**
 * @file
 *
 * This configuration file manages the settings for the provision back end of the Aegir hosting system.
 *
 * If you are not using the hosting front end, you should copy this file to the root of your
 * Drupal installation and change the settings accordingly.
 *
 * For the most parts the defaults should be sensible and just work.
 *
 * @deprecated
 *   Note that most of the settings here are deprecated and really not
 *   supported anymore since the switch to drush aliases for storage.
 */

/**
 * Directory to store site backups
 *
 * If you have multiple provision managed platforms on this system, it is highly
 * recommended to use the same path for each platform you have.
 */
# $options['backup_path'] = '/var/aegir/backups';

/**
 * Directory to store configuration files
 *
 * All system related configuration files will be stored here.
 *
 * If you have multiple provision managed platforms on this server, it is highly
 * recommended to use the same path for each platform you have.
 */
# $options['config_path'] = '/var/aegir/config';

/**
 * The login name for the shell user who will be running the provision scripts
 *
 * This needs to be a user account that is not root or the web server user.
 */
# $options['script_user'] = 'aegir';

/**
 * The group that the web server is running as
 */
# $options['web_group'] = 'apache';

/**
 * Database credentials for a database account capable of creating databases and users
 */
# $options['master_db'] = 'mysql://aegir:password@localhost/mysql';

/**
 * Command to restart apache when an action has been completed
 */
# $options['restart_cmd'] = 'sudo /usr/sbin/apachectl restart';

/**
 * The address of the Hostmaster installation
 * This url will be used to redirect sites that haven't been found or have been disabled.
 */
# $options['master_url'] = 'http://aegir.example.com';

/**
 * The database prefix to use instead of the default 'site_'
 */
# $options['aegir_db_prefix'] = 'prefix_';

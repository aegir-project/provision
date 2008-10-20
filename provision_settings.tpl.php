/**
 * This configuration file manages the settings for the provisionining back end of the Aegir hosting system.
 *
 * This file was automatically created for you by the provision setup functionality, to test the settings in
 * this file, please run the provision verify command.
 *
 */

/**
 * Directory to store site backups.
 *
 * If you have multiple provision managed platforms on this system, it is highly
 * recommended to use the same path for each platform you have.
 */
define('PROVISION_BACKUP_PATH', '<?php print $backup_path ?>');

/**
 * Directory to store configuration files
 *
 * All system related configuration files will be stored here.
 *
 * If you have multiple provision managed platforms on this server, it is highly
 * recommended to use the same path for each platform you have.
 */
define('PROVISION_CONFIG_PATH', '<?php print $config_path ?>');


/**
 * The login name for the shell user who will be running the provision scripts
 *
 * This needs to be a user account that is not root or the web server user.
 */
define('PROVISION_SCRIPT_USER', '<?php print $script_user ?>');

/**
 * The group that the web server is running as
 */
define('PROVISION_WEB_GROUP', '<?php print $web_group ?>');

/**
 * Database credentials for a database account capable of creating databases and users
 */
define('PROVISION_MASTER_DB', '<?php print $master_db ?>');

/**
 * Command to restart apache when an action has been completed
 */
define('PROVISION_RESTART_CMD', '<?php print $restart_cmd ?>');

/**
 * The address of the Hostmaster installation
 * This url will be used to redirect sites that haven't been found or have been disabled.
 */
define('PROVISION_MASTER_URL', '<?php print $master_url ?>');

<?php
/**
 * @file
 * The provisioning framework error code definitions.
 */


/**
* @name Error status definitions
* @{
* Bitmask values used to generate the error code to return.
* @see provision_set_error(), provision_get_error(), provision_cmp_error()
*/

/** Succesful **/
define('PROVISION_SUCCESS', -1);
/** Database could not be accessed, or configured */
define('PROVISION_DB_ERROR', 1);
/** Drupal was unable to complete it's installation */
define('PROVISION_INSTALL_ERROR', 2);
/** Could not create files due to permission error - potentially less severe */ 
define('PROVISION_PERM_ERROR', 4); 
/** Web server could not be restarted, or other server related issues - less severe */
define('PROVISION_WEB_ERROR', 8);
/** To be used while testing if the provision framework is actually working. */
define('PROVISION_FRAMEWORK_ERROR', 16);
/** When the site is not available on this platform, or has not been installed. */
define('PROVISION_SITE_NOT_FOUND', 32);
/** When the site is already installed, ie: there is a conflict. */
define('PROVISION_SITE_INSTALLED', 64);

#  List of provisioning errors. This is a global so that other modules can reference it.
global $provision_errors;
$provision_errors = array(
  PROVISION_SUCCESS => T("Successful"),
  PROVISION_DB_ERROR => t("Database error"),
  PROVISION_INSTALL_ERROR => t("Drupal installation error"),
  PROVISION_PERM_ERROR => t("File permission error"),
  PROVISION_WEB_ERROR => t("Web server error"),
  PROVISION_FRAMEWORK_ERROR => t("Provision framework error"),
  PROVISION_SITE_NOT_FOUND => t("Site not found"),
  PROVISION_SITE_INSTALLED => t("Site has already been installed"),  
  );
/**
 * @} End of "name Error status defintions".
 */

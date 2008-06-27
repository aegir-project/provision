<?php

/**
 * @defgroup pathhandling Managing paths, permissions and file ownership
 *
 * This group provides an interface to common path handling operations, through
 * the provision_path helper function, which will take care of verification and
 * any error logging required.
 */

/**
 * Perform tasks on a path.
 *
 * Perform tasks on a path, and logs error messages / codes on success or failure.
 * This function will call another function which defines the functionality,
 * and exists to provide a consistent interface for file operations with error logging
 * integration.
 *
 * Many of the provision_path_$op functions are really simple, but are wrapped
 * in functions to provide a consistent interface for provision_path to operate
 * with.
 *
 * @param type
 *    The type of operation to perform. One of the following:
 *      writable - The $path can be written to.
 *      exists - The $path exists.
 *      is_dir - The $path is a directory.
 *      readable - The $path is readable.
 *      owner - The $path belongs to the user in $confirm.
 *      group - The $path belongs to the group in $confirm.
 *      mkdir - Create the $path directory.
 *      unlink - Delete the file $path.
 *      rmdir - Delete the directory $path.
 *      chmod - Change the file permissions of $path to the octal value in $confirm.
 *      chown - Change the owner of $path to the user in $confirm.
 *      chgrp - Change the group of $path to the group in $confirm.
 *      switch_paths - Move $path to $confirm, and vice versa.
 *
 * @param path
 *    The path you want to perform the file operation on.
 *
 * @param confirm
 *    Confirm that the final value of the file operation matches this value.
 *    This value defaults to true, which is sufficient for most file operations.
 *
 *    Certain tasks such as chmod, chown and chgp will attempt to change the
 *    properties of $path to match the value in $confirm, and then test that
 *    the change was completed succesfully afterwards.
 *    
 *    These exceptions are :
 *      chmod - $confirm is an octal value denoting the desired file permissions.
 *      chown - $confirm is the name or user id you wish to change the file ownership to.
 *      chgrp - $confirm is the name of group id you wish to change the file group ownership to.
 *      switch_paths - $confirm is the path that you want to replace the $path with.
 *
 * @param succeed_message
 *    Log this as a notice into the logging system, if the operation completed succesfully.
 *
 * @param fail_message
 *    Log this as a error to the logging system, if the $error_codes parameter has been set,
 *    otherwise, log this as a warning. If the operation specifies an additional reason for
 *    the operation failing, it will be appended to this message.
 *
 * @param error_codes
 *    Generate these system level errors using the provision error bitmasks.
 *
 * @return
 *    Returns TRUE if the test against $confirm passed, otherwise returns FALSE.
 */
function provision_path($op, $path, $confirm = true, $succeed_message = '', $fail_message = '', $error_codes = null) {
  # The code style is a bit weird here, but it's a bit easier to read this way.
  $func = "provision_path_" . $op;
  if (function_exists($func)) {
    // The reason variable is passed into the operation function, to allow the function
    // to specify an additional reason as to why the operation failed.
    $reason = '';

    $value = $func($path, $confirm, $reason);

    clearstatcache(); // this needs to be called, otherwise we get the old info 
    $tokens = array("@path" => $path, "@op" => $op, "@confirm" => $confirm);
    if ($reason) {
      $fail_message = $fail_message . " (" . $reason . ")";
    }
    $status = ($value == $confirm);
    if ($status) {
      if ($succeed_message) {
        provision_log("message", t($succeed_message, $tokens));      
      }
    }
    else {
      if ($error_codes) {
        // Trigger a sysem halting error
        provision_log("error", t($fail_message, $tokens));
        provision_set_error($error_codes);
      }
      else {
        // Trigger a warning
        provision_log("warning", t($fail_message, $tokens));
      }
    }
    return $status;
  }
}


function provision_path_writable($path) {
  return is_writable($path);
}

function provision_path_exists($path) {
  return file_exists($path);
}

function provision_path_is_dir($path) {
  return is_dir($path);
}

function provision_path_readable($path) {
  return is_readable($path);
}

function provision_path_owner($path) {
  $info = posix_getpwuid(fileowner($path));
  return $info['name'];
}

function provision_path_group($path) {
  return filegroup($path);
}

function provision_path_mkdir($path) {
  return mkdir($path, 0770, true);
}

function provision_path_rmdir($path) {
  return (file_exists($path) && is_dir($path)) ? rmdir($path) : false;
}

function provision_path_unlink($path) {
  return (file_exists($path)) ? unlink($path) : false;
}


/*
 * This is where the more complex file operations start
 */

function provision_path_chmod($path, &$perms, &$reason) {
  if (!chmod($path, $perms)) {
    $reason = t('chmod failed');
    return false;
  }
  clearstatcache(); // this needs to be called, otherwise we get the old info 
  $perms = sprintf('%o', $perms); # needs to be reset to oct
  $value = substr(sprintf('%o', fileperms($path)), -4);
  return $value;
}

function provision_path_chown($path, &$owner, &$reason) {
  if ($owner = provision_posix_username($owner)) {
    if (!chown($path, $owner)) {
      $reason = t("chmod failed"); 
    }
  }
  else {
    $reason = t("the user does not exist");
  }

  clearstatcache(); // this needs to be called, otherwise we get the old info 
  return provision_posix_username(fileowner($path));
}

function provision_path_chgrp($path, &$group, &$reason) {
  if ($group = provision_posix_groupname($group)) {
    if (provision_user_in_group(PROVISION_SCRIPT_USER, $group)) {
      if (!chgrp($path, $group)) {
        $reason = t("chgrp failed"); 
      }
    }
    else {
      $reason = t("@user is not in @group group", array("@user" => PROVISION_SCRIPT_USER, "@group" => $group));
    }
  }
  else {
    $reason = t("the group does not exist");
  }

  clearstatcache(); // this needs to be called, otherwise we get the old info 
  return provision_posix_groupname(filegroup($path));
}

function provision_path_switch_paths($path1, &$path2, &$reason) {
  //TODO : Add error reasons.
  $temp = $path1 . '.tmp';
  if (rename($path1, $temp)) { 
    if (rename($path2, $path1)) {
      if (rename($temp, $path2)) {
        return $path2; // path1 is now path2
      }
      else {
        // same .. just in reverse
        rename($path1, $path2);
        rename($temp, $path1);
      }
    }
    else {
      // same .. just in reverse
      rename($temp, $path1);
    }   

  }
  return $path1;
}

function provision_path_extract($path, &$target, &$reason) {
  if (file_exists($path) && is_readable($path)) {
    if (is_writeable(dirname($target)) && !file_exists($target) && !is_dir($target)) {
      mkdir($target);
      provision_shell_exec("tar -zxf %s -C %s", $path, $target);
      provision_log("notice", sprintf("Running: tar -zxf %s -C %s", $path, $target));
      if (is_writeable(dirname($target)) && is_readable(dirname($target)) && is_dir($target)) {
        $target = TRUE;
        return TRUE;
      }
      else {
        $reason = t("The file could not be extracted");
      }
    }
    else {
      $reason = t("The target directory could not be written to");
      return false;
    }
  }
  else {
    $reason = t("Backup file could not be opened");
    return false;
  }

}

/**
*@} end filegroup
 */


/**
 * Small helper function for creation of configuration directories.
 */
function _provision_create_dir($path, $name, $perms) {
  $exists = provision_path("exists",$path, true ,
    $name . ' ' . t("path exists."),
    $name . ' ' . t("path does not exist.")
  );

  if (!$exists) {
    $exists = provision_path("mkdir", $path, true,
      $name . ' ' . t("path has been created."),
      $name . ' ' . t("path could not be created."),
      PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR);
  }

  if ($exists) {
    provision_path("chown", $path, PROVISION_SCRIPT_USER, 
      $name . ' ' . t("ownership of path has been changed to @confirm."),
      $name . ' ' . t("ownership of path could not be changed to @confirm."),
      PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR);

    provision_path("chmod", $path, $perms, 
      $name . ' ' . t("permissions of path have been changed to @confirm."),
      $name . ' ' . t("permissions of path could not be changed to @confirm."),
      PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR );

    $writable = provision_path("writable", $path, true,
        $name . ' ' . t("path is writeable."),
        $name . ' ' . t("path is not writeable."),
        PROVISION_PERM_ERROR | PROVISION_FRAMEWORK_ERROR);

  }

  return ($exists && $writable);
}


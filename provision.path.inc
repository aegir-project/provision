<?php
// $Id$
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
 *      symlink - Create a symlink from $path to $confirm.
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
 *    This value defaults to TRUE, which is sufficient for most file operations.
 *
 *    Certain tasks such as chmod, chown and chgp will attempt to change the
 *    properties of $path to match the value in $confirm, and then test that
 *    the change was completed succesfully afterwards.
 *    
 *    These exceptions are :
 *      symlink - $confirm is the path to the symlink being created
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
function provision_path($op, $path, $confirm = TRUE, $succeed_message = NULL, $fail_message = NULL, $error_codes = NULL) {
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
      if (!is_null($succeed_message)) {
        drush_log(dt($succeed_message, $tokens), 'message');      
      }
    }
    else {
      if ($error_codes) {
        // Trigger a sysem halting error
        if (!is_null($fail_message)) {
          drush_set_error($error_codes, dt($fail_message, $tokens));
        }
        else {
          drush_set_error($error_codes);
        }
      }
      else {
        // Trigger a warning
        if (!is_null($fail_message)) {
          drush_log(dt($fail_message, $tokens), 'warning');
        }
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
  if (version_compare(PHP_VERSION, '5.0.0', '<')) {
    return _provision_mkdir_recursive($path, 0770);
  } else {
    return mkdir($path, 0770, TRUE);
  }
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

function provision_path_chmod($path, &$perms, &$reason, $recursive = FALSE) {
  $func = ($recursive) ? '_provision_chmod_recursive' : 'chmod';

  if (!@$func($path, $perms)) {
    $reason = dt('chmod to @perm failed on @path', array('@perm' => sprintf('%o', $perms), '@path' => $path));
    return false;
  }
  clearstatcache(); // this needs to be called, otherwise we get the old info 
  $value = substr(sprintf('%o', fileperms($path)), -4);
  $perms = sprintf('%04o', $perms);
  return $value;
}

function provision_path_chown($path, &$owner, &$reason, $recursive = FALSE) {
  $func = ($recursive) ? '_provision_chown_recursive' : 'chown';
  if ($owner = provision_posix_username($owner)) {
    if (!$func($path, $owner)) {
      $reason = dt("chown to @owner failed on @path", array('@owner' => $owner, '@path' => $path)) ; 
    }
  }
  else {
    $reason = dt("the user does not exist");
  }

  clearstatcache(); // this needs to be called, otherwise we get the old info 
  return provision_posix_username(fileowner($path));
}

function provision_path_chgrp($path, &$gid, &$reason, $recursive = FALSE) {
  $func = ($recursive) ? '_provision_chgrp_recursive' : 'chgrp';
  if ($group = provision_posix_groupname($gid)) {
    if (provision_user_in_group(drush_get_option('script_user'), $gid)) {
      if ($func($path, $group)) {
        return $group;
      }
      else {
        $reason = dt("chgrp to @group failed on @path", array('@group' => $group, '@path' => $path));
      }
    }
    else {
      $reason = dt("@user is not in @group group", array("@user" => drush_get_option('script_user'), "@group" => $group));
    }
  }
  elseif (!@$func($path, $gid)) { # try to change the group anyways
    $reason = dt("the group does not exist");
  }

  clearstatcache(); // this needs to be called, otherwise we get the old info 
  return provision_posix_groupname(filegroup($path));
}


function provision_path_chmod_recursive($path, &$perms, &$reason) {
  return provision_path_chmod($path, $perms, $reason, TRUE);
}

function provision_path_chown_recursive($path, &$owner, &$reason) {
  return provision_path_chown($path, $owner, $reason, TRUE);
}

function provision_path_chgrp_recursive($path, &$gid, &$reason) {
  return provision_path_chgrp($path, $gid, $reason, TRUE);
}


function provision_path_switch_paths($path1, &$path2, &$reason) {
  //TODO : Add error reasons.
  $temp = $path1 .'.tmp';
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
      provision_shell_exec("tar -zpxf %s -C %s", $path, $target);
      drush_log(sprintf("Running: tar -zpxf %s -C %s", $path, $target));
      if (is_writeable(dirname($target)) && is_readable(dirname($target)) && is_dir($target)) {
        $target = TRUE;
        return TRUE;
      }
      else {
        $reason = dt("The file could not be extracted");
      }
    }
    else {
      $reason = dt("The target directory could not be written to");
      return false;
    }
  }
  else {
    $reason = dt("Backup file could not be opened");
    return false;
  }

}

function provision_path_symlink($path, &$target, &$reason) {
  if (file_exists($target) && !is_link($target)) {
    $reason = dt("A file already exists at @path");
    return FALSE;
  }
  if (is_link($target) && (readlink($target) != $path)) {
    $reason = dt("A symlink already exists at target, but it is pointing to @link", array("@link" => readlink($target)));
    return FALSE;
  }
  if (is_link($target) && (readlink($target) == $path)) {
    $target = TRUE;
    return TRUE;
  }
  if (symlink($path, $target)) {
    $target = TRUE;
    return TRUE;
  }
  else {
    $reason = dt('The symlink could not be created, an error has occured');
    return FALSE;
  }


}

/**
*@} end filegroup
 */

/**
 * Small helper function for creation of configuration directories.
 */
function _provision_create_dir($path, $name, $perms) {
  $exists = provision_path("exists",$path, TRUE ,
    $name . ' ' . dt("path exists."),
    $name . ' ' . dt("path does not exist.")
  );

  if (!$exists) {
    $exists = provision_path("mkdir", $path, TRUE,
      $name . ' ' . dt("path has been created."),
      $name . ' ' . dt("path could not be created."),
      'DRUSH_PERM_ERROR');
  }

  if ($exists) {
    provision_path("chown", $path, drush_get_option('script_user'), 
      $name . ' ' . dt("ownership of path has been changed to @confirm."),
      $name . ' ' . dt("ownership of path could not be changed to @confirm."),
      'DRUSH_PERM_ERROR');

    provision_path("chmod", $path, $perms, 
      $name . ' ' . dt("permissions of path have been changed to @confirm."),
      $name . ' ' . dt("permissions of path could not be changed to @confirm."),
      'DRUSH_PERM_ERROR' );

    $writable = provision_path("writable", $path, TRUE,
        $name . ' ' . dt("path is writable."),
        $name . ' ' . dt("path is not writable."),
        'DRUSH_PERM_ERROR');

  }

  return ($exists && $writable);
}

/**
 * Makes directory recursively, returns TRUE if exists or made (for PHP4 compatibility)
 * Code courtesy of: http://ca3.php.net/manual/en/function.mkdir.php#81656
 *
 * @param string $pathname The directory path.
 * @return boolean returns TRUE if exists or made or FALSE on failure.
 */
function _provision_mkdir_recursive($path, $mode) {
  is_dir(dirname($path)) || _provision_mkdir_recursive(dirname($path), $mode);
  return is_dir($path) || mkdir($path, $mode);
}

/**
 * Chmod a directory recursively
 */
function _provision_chmod_recursive($path, $filemode) {
 if ( !is_dir($path) ) {
  return chmod($path, $filemode);
 }
 else {
   $output = 0;
   $cmd = sprintf("chmod -R %o %s", $filemode, $path);
   system($cmd, $output);
   return !$output; 
 }
}

/**
 * Chown a directory recursively
 */
function _provision_chown_recursive($path, $owner) {
 if ( !is_dir($path) ) {
  return chown($path, $owner);
 }
 else {
   $output = 0;
   $cmd = sprintf("chown -R %s %s", $owner, $path);
   system($cmd, $output);
   return !$output; 
 }
}


/**
 * Chgrp a directory recursively
 */
function _provision_chgrp_recursive($path, $owner) {
 if ( !is_dir($path) ) {
  return chgrp($path, $owner);
 }
 else {
   $output = 0;
   $cmd = sprintf("chgrp -R %s %s", $owner, $path);
   system($cmd, $output);
   return !$output; 
 }
}

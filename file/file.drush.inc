<?php
// $Id$
/**
 * @file Filesystem access module.
 *
 * Handle writing and syncing configuation files across multiple servers.
 * Provides an interface to common path handling operations, through the path
 * helper method, which will take care of verification and any error logging
 * required.
 */

include_once(dirname(__FILE__) . '/../provision.service.inc');

function file_provision_services() {
  return array('file' => 'local');
}


function file_drush_exit() {
  // todo sync out to other servers
  //provision_service('file')->sync();
}

class provisionService_file_local extends provisionService_file {

}


class provisionService_file extends provisionService {
  /**
   * Perform tasks on a path.
   *
   * Perform tasks on a path, and logs error messages / codes on success or
   * failure.  This function will call another method which defines the
   * functionality, and exists to provide a consistent interface for file
   * operations with error logging integration.
   *
   * Many of the $op methods are really simple, but are wrapped in methods to
   * provide a consistent interface for path to operate with.
   *
   * @param type
   *    The type of operation to perform. One of the following:
   *      fopen - Get a file pointer for writing.
   *
   * @param path
   *    The path you want to perform the file operation on.
   *
   * @param confirm
   *    Confirm that the final value of the file operation matches this value.
   *    This value defaults to TRUE, which is sufficient for most file operations.
   *
   *    These exceptions are :
   *      fopen - $confirm is the $mode
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
  function path($op, $path, $confirm = TRUE, $succeed_message = NULL, $fail_message = NULL, $error_codes = NULL) {
    if (method_exists($this, $op)) {
      // The reason variable is passed into the operation function, to allow the function
      // to specify an additional reason as to why the operation failed.
      $reason = '';

      $value = $this->$op($path, $confirm, $reason);

      clearstatcache(); // this needs to be called, otherwise we get the old info 
      $tokens = array("@path" => $path, "@op" => $op, "@confirm" => $confirm);
      if ($reason) {
        $fail_message .= " (" . $reason . ")";
      }
      $status = is_array($value) ? ($value[0] == $confirm) : ($value == $confirm);
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
      return is_array($value) ? $value[1] : $status;
    }
  }

  /**
   * Determine if $path can be written to.
   *
   * Sets @path token for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   */
  function writable($path) {
    $this->_clear_state();

    $this->last_status = is_writable($path);
    $this->tokens = array('@path' => $path);

    return $this;
  }

  /**
   * Determine if $path exists.
   *
   * Sets @path token for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   */
  function exists($path) {
    $this->_clear_state();

    $this->last_status = file_exists($path);
    $this->tokens = array('@path' => $path);

    return $this;
  }

  /**
   * Determine if $path is readable.
   *
   * Sets @path token for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   */
  function readable($path) {
    $this->_clear_state();

    $this->last_status = is_readable($path);
    $this->tokens = array('@path' => $path);

    return $this;
  }

  /**
   * Create the $path directory.
   *
   * Sets @path token for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   */
  function mkdir($path) {
    $this->_clear_state();

    $this->last_status = mkdir($path, 0770, TRUE);
    $this->tokens = array('@path' => $path);

    return $this;
  }

  /**
   * Delete the directory $path.
   *
   * Sets @path token for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   */
  function rmdir($path) {
    $this->_clear_state();

    $this->last_status = rmdir($path);
    $this->tokens = array('@path' => $path);

    return $this;
  }

  /**
   * Delete the file $path.
   *
   * Sets @path token for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   */
  function unlink($path) {
    $this->_clear_state();

    $this->last_status = unlink($path);
    $this->tokens = array('@path' => $path);

    return $this;
  }

  function fopen($path, $mode) {
    $fp = fopen($path, $mode);
    if ($fp === FALSE) {
      return FALSE;
    }
    return array(TRUE, $fp);
  }

  /**
   * Change the file permissions of $path to the octal value in $perms.
   *
   * @param $perms
   *   An octal value denoting the desired file permissions.
   */
  function chmod($path, $perms, $recursive = FALSE) {
    $this->_clear_state();

    $this->tokens = array('@path' => $path, '@perm' => sprintf('%o', $perms));

    $func = ($recursive) ? array($this, '_chmod_recursive') : 'chmod';
    if (!@call_user_func($func, $path, $perms)) {
      $this->tokens['@reason'] = dt('chmod to @perm failed on @path', array('@perm' => sprintf('%o', $perms), '@path' => $path));
      return false;
    }
    clearstatcache(); // this needs to be called, otherwise we get the old info 
    $this->last_status = substr(sprintf('%o', fileperms($path)), -4) == sprintf('%04o', $perms);

    return $this;
  }

  /**
   * Change the owner of $path to the user in $owner.
   *
   * Sets @path, @owner, and @reason tokens for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   * @param $owner
   *   The name or user id you wish to change the file ownership to.
   * @param $recursive
   *   TRUE to descend into subdirectories.
   */
  function chown($path, $owner, $recursive = FALSE) {
    $this->_clear_state();

    $this->tokens = array('@path' => $path, '@gid' => $gid);

    $func = ($recursive) ? array($this, '_chown_recursive') : 'chown';
    if ($owner = provision_posix_username($owner)) {
      if (!call_user_func($func, $path, $owner)) {
        $this->tokens['@reason'] = dt("chown to @owner failed on @path", array('@owner' => $owner, '@path' => $path)) ; 
      }
    }
    else {
      $this->tokens['@reason'] = dt("the user does not exist");
    }

    clearstatcache(); // this needs to be called, otherwise we get the old info 
    $this->last_status = $owner == provision_posix_username(fileowner($path));

    return $this;
  }

  /**
   * Change the group of $path to the group in $gid.
   *
   * Sets @path, @gid, and @reason tokens for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   * @param $gid
   *   The name of group id you wish to change the file group ownership to.
   * @param $recursive
   *   TRUE to descend into subdirectories.
   */
  function chgrp($path, $gid, $recursive = FALSE) {
    $this->_clear_state();

    $this->tokens = array('@path' => $path, '@gid' => $gid);

    $func = ($recursive) ? array($this, '_chgrp_recursive') : 'chgrp';
    if ($group = provision_posix_groupname($gid)) {
      if (provision_user_in_group(drush_get_option('script_user'), $gid)) {
        if (!call_user_func($func, $path, $group)) {
          $this->tokens['@reason'] = dt("chgrp to @group failed on @path", array('@group' => $group, '@path' => $path));
        }
      }
      else {
        $this->tokens['@reason'] = dt("@user is not in @group group", array("@user" => drush_get_option('script_user'), "@group" => $group));
      }
    }
    elseif (!@call_user_func($func, $path, $gid)) { # try to change the group anyways
      $this->tokens['@reason'] = dt("the group does not exist");
    }

    clearstatcache(); // this needs to be called, otherwise we get the old info 
    $this->last_status = $group == provision_posix_groupname(filegroup($path));

    return $this;
  }

  /**
   * Move $path1 to $path2, and vice versa.
   *
   * @param $path1
   *   The path that you want to replace the $path2 with.
   * @param $path2
   *   The path that you want to replace the $path1 with.
   */
  function switch_paths($path1, $path2) {
    $this->_clear_state();

    $this->tokens = array('@path1' => $path1, '@path2' => $path2);

    //TODO : Add error reasons.
    $temp = $path1 . '.tmp';
    if (!file_exists($path1)) {
      $this->last_status = rename($path2, $path1);
    }
    elseif (!file_exists($path2)) {
      $this->last_status = rename($path1, $path2);
    }
    elseif (rename($path1, $temp)) { 
      if (rename($path2, $path1)) {
        if (rename($temp, $path2)) {
          $this->last_status = $path2; // path1 is now path2
        }
        else {
          // same .. just in reverse
          $this->last_status = rename($path1, $path2) && rename($temp, $path1);
        }
      }
      else {
        // same .. just in reverse
        $this->last_status = rename($temp, $path1);
      }   
    }
    $this->last_status = FALSE;

    return $this;
  }

  /**
   * Extract gzip-compressed tar archive.
   *
   * Sets @path, @target, and @reason tokens for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to extract.
   * @param $target
   *   The destination path to extract to.
   */
  function extract($path, $target) {
    $this->_clear_state();

    $this->tokens = array('@path' => $path, '@target' => $target);

    if (file_exists($path) && is_readable($path)) {
      if (is_writeable(dirname($target)) && !file_exists($target) && !is_dir($target)) {
        $this->mkdir($target);
        $oldcwd = getcwd();
        // we need to do this because some retarded implementations of tar (e.g. SunOS) don't support -C
        chdir($target);
        // same here: some do not support -z
        $command = 'gunzip -c %s | tar pxf -';
        drush_log(dt('Running: %command in %target', array('%command' => sprintf($command, $path), '%target' => $target)));
        $result = provision_shell_exec($command, $path);
        chdir($oldcwd);

        if ($result && is_writeable(dirname($target)) && is_readable(dirname($target)) && is_dir($target)) {
          $this->last_status = TRUE;
        }
        else {
          $this->tokens['@reason'] = dt('The file could not be extracted');
          $this->last_status = FALSE;
        }
      }
      else {
        $this->tokens['@reason'] = dt('The target directory could not be written to');
        $this->last_status = FALSE;
      }
    }
    else {
      $this->tokens['@reason'] = dt('Backup file could not be opened');
      $this->last_status = FALSE;
    }

    return $this;
  }

  /**
   * Create a symlink from $path to $target.
   *
   * Sets @path, @target, and @reason tokens for ->succeed and ->fail.
   *
   * @param $path
   *   The path you want to perform this operation on.
   * @param $target
   *   The path you want the link to point to.
   */
  function symlink($path, $target) {
    $this->_clear_state();

    $this->tokens = array('@path' => $path, '@target' => $target);

    if (file_exists($target) && !is_link($target)) {
      $this->tokens['@reason'] = dt("A file already exists at @path");
      $this->last_status = FALSE;
    }
    elseif (is_link($target) && (readlink($target) != $path)) {
      $this->tokens['@reason'] = dt("A symlink already exists at target, but it is pointing to @link", array("@link" => readlink($target)));
      $this->last_status = FALSE;
    }
    elseif (is_link($target) && (readlink($target) == $path)) {
      $this->last_status = TRUE;
    }
    elseif (symlink($path, $target)) {
      $this->last_status = TRUE;
    }
    else {
      $this->tokens['@reason'] = dt('The symlink could not be created, an error has occured');
      $this->last_status = FALSE;
    }

    return $this;
  }

  /**
   * Small helper function for creation of configuration directories.
   */
  function create_dir($path, $name, $perms) {
    $exists = $this->exists($path)
      ->succeed($name . ' path @path exists.')
      ->fail($name . ' path @path does not exist.')
      ->status();

    if (!$exists) {
      $exists = $this->mkdir($path)
        ->succeed($name . ' path @path has been created.')
        ->fail($name . ' path @path could not be created.', 'DRUSH_PERM_ERROR')
        ->status();
    }

    if ($exists) {
      $this->chown($path, drush_get_option('script_user'))
        ->succeed($name . ' ownership of path has been changed to @perm.')
        ->fail($name . ' ownership of path could not be changed to @perm.', 'DRUSH_PERM_ERROR');

      $this->chmod($path, $perms)
        ->succeed($name . ' permissions of path have been changed to @perm.')
        ->fail($name . ' permissions of path could not be changed to @perm.', 'DRUSH_PERM_ERROR');

      $this->writable($path)
        ->succeed($name . ' path @path is writable.')
        ->fail($name . ' path @path is not writable.', 'DRUSH_PERM_ERROR');
    }

    return $exists;
  }

  /**
   * Walk the given tree recursively (depth first), calling a function on each file
   *
   * $func is not checked for existence and called directly with $path and $arg
   * for every file encountered.
   *
   * @param string $func a valid callback, usually chmod, chown or chgrp
   * @param string $path a path in the filesystem
   * @param string $arg the second argument to $func
   * @return boolean returns TRUE if every $func call returns true
   */
  function _call_recursive($func, $path, $arg) {
    $status = 1;
    // do not follow symlinks as it could lead to a DOS attack
    // consider someone creating a symlink from files/foo to ..: it would create an infinite loop
    if (!is_link($path) && ($dh = @opendir($path))) {
      while (($file = readdir($dh)) !== false) {
        if ($file != '.' && $file != '..') {
          $status = $this->_call_recursive($func, $path . "/" . $file, $arg) && $status;
        }
      }
      closedir($dh);
    }
    $status = call_user_func($func, $path, $arg) && $status;
    return $status;
  }

  /**
   * Chmod a directory recursively
   *
   */
  function _chmod_recursive($path, $filemode) {
    return $this->_call_recursive('chmod', $path, $filemode);
  }

  /**
   * Chown a directory recursively
   */
  function _chown_recursive($path, $owner) {
    return $this->_call_recursive('chown', $path, $owner);
  }

  /**
   * Chgrp a directory recursively
   */
  function _chgrp_recursive($path, $group) {
    return $this->_call_recursive('chgrp', $path, $group);
  }
}

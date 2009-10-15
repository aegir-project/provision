<?php
// $Id$

/**
 * @file
 *
 * This include file is based on the cvs_deploy module by Derek Wright.
 * @author Derek Wright ("dww") http://drupal.org/user/46549
 */

/**
 * Used by provision to hook into the cvs_deploy system.
 */
function _provision_cvs_deploy(&$file) {
  $file->filename = realpath($file->filename);
  $project = (isset($file->info['project'])) ? $file->info['project'] : _provision_cvs_deploy_get_project_name($file);
  $file->project = $project;
  _provision_cvs_deploy_version_alter($file->info['version'], $file);
  $file->version = $file->info['version'];
}

/**
 * Private helper to alter the 'project' of a module based on what directory
 * in the CVS repository the module has been checked out from.
 */
function _provision_cvs_deploy_get_project_name($file) {
  static $projects = array();
  $name = $file->name;
  if (empty($projects[$name])) {
    // TODO: cache this in {cache}, too?
    $cvs_dir = dirname($file->filename) .'/CVS';
    if (is_dir($cvs_dir)) {
      $repository = '';
      if (file_exists($cvs_dir .'/Repository')) {
        $repo_file = trim(file_get_contents($cvs_dir .'/Repository'));
        if ($repo_file) {
          $parts = explode('/', $repo_file);
          if ($parts[0] == 'drupal') {
            $projects[$name] = $parts[0];
          }
          else {
            $projects[$name] = $parts[2];
          }
        }
      }
    }
  }
  return (isset($projects[$name]) ? $projects[$name] : '');
}

/**
 * Private helper to alter the version of a module based on what we can figure
 * out about the CVS tag in use.
 */
function _provision_cvs_deploy_version_alter(&$version, $file) {
  static $available = array();
  $match = array();
  if (empty($version)) {
    // The .info file contains no version data. Find the version based
    // on the sticky tag in the local workspace (the CVS/Tag file).
    $cvs_dir = dirname($file->filename) .'/CVS';
    if (is_dir($cvs_dir)) {
      $tag = '';  // If there's no Tag file, there's no tag, a.k.a. HEAD.
      if (file_exists($cvs_dir .'/Tag')) {
        $tag_file = trim(file_get_contents($cvs_dir .'/Tag'));
        if ($tag_file) {
          // Get the sticky tag for this workspace: strip off the leading 'T'.
          $tag = preg_replace('@^(T|N)@', '', $tag_file);
        }
      }
      $version = _provision_cvs_deploy_version_from_tag($tag);
    }
  }
  // The weird concatenation prevents CVS from 'expanding' this $Name.
  elseif (preg_match('/\$'.'Name: (.*?)\$/', $version, $match)) {
    $version = _provision_cvs_deploy_version_from_tag(trim($match[1]));
  }
}

/**
 * Returns the human-readable version string from a given CVS tag.
 */
function _provision_cvs_deploy_version_from_tag($tag) {
  // If there's nothing, it must be a HEAD checkout, and therefore,
  // we have no idea what the version is.
  if (!$tag || $tag == 'HEAD') {
    $version = drush_drupal_major_version() . '.x-dev';
  }
  // See if it's a Drupal core release
  elseif (preg_match('@^DRUPAL-(\d+)-(\d+)$@', $tag, $match)) {
    $version = $match[1] . '.' . $match[2];
  }
  // See if it's a full, official release from a tag:
  elseif (preg_match('@^DRUPAL-(\d+)--(\d+)-(\d+)(-.+)?@', $tag, $match)) {
    $version = $match[1] .'.x-'. $match[2] .'.'. $match[3];
    if (isset($match[4])) {
      // This version's tag has 'extra', so clean that up.
      $version .= '-'. preg_replace('/[_-]/', '', strtolower($match[4]));
    }
  }
  // If not, see if it's from a branch (like a development snapshot).
  elseif (preg_match('@^DRUPAL-(\d+)(--(\d+))?@', $tag, $match)) {
    $version = $match[1] .'.x-'. (isset($match[3]) ? $match[3] : '1') .'.x-dev';
  }
  return $version;
}

<?php
// $Id$

/**
 * @file Provision named context base classes.
 */

/**
 * Store and access context objects by alias name.
 *
 * @param $name
 *   A Drush alias name, including leading @.
 * @param $_root_object
 *   Internal use only, set default object returned by d().
 *
 * @return
 *   Provision_Context object.
 */
function & d($name = NULL, $_root_object = FALSE) {
  static $instances = null;
  static $default_instance = '@self';

  if (is_object($name)) {
    return $name;
  }

  if ($name == 'all') {
    return $instances;
  }

  if (is_null($name)) {
    return $instances[$default_instance];
  }
  else {
    if ($_root_object) {
      $default_instance = $name;
    }

    if (isset($instances[$name])) {
      return $instances[$name];
    }
    else {
      $instances[$name] = provision_context_factory($name);
      $instances[$name]->method_invoke('init');
      $instances[$name]->type_invoke('init');

      return $instances[$name];
    }
  }
}

/**
 * Simple access layer for drush_sitealias_get_record.
 *
 * Everytime sitealiases are fetched a lot of processing happens, but if the
 * file doesnt exist yet there's a whole lot of unnecesary stuff happening. 
 *
 * We cache the result locally here.
 */
function provision_sitealias_get_record($name) {
  static $cache = array();

  if (!isset($cache[$name])) {
    $cache[$name] = drush_sitealias_get_record($name);
  }

  return $cache[$name];
}

/**
 * Create a new context object and cache it in the d() static cache
 * function.
 */
function provision_context_factory($name) {
  // the default type, can also be 'platform' or 'site'
  $type = 'server';

  $record = provision_sitealias_get_record($name);
  $options = array_merge(drush_get_context('stdin'), drush_get_context('options'), drush_get_context('cli'));

  if (isset($record['context_type'])) {
    $type = $record['context_type'];
  }
  elseif (isset($options['context_type'])) {
    $type = $options['context_type'];
  }
  $classname = "Provision_Context_{$type}";

  return new $classname($name);
}

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
 * @param $allow_creation
 *   Defaults to TRUE. Allows creating a new context object with the specified
 *   $name.
 *
 * @return
 *   provision_Context object or NULL if it can't be loaded and
 *   $allow_creation == FALSE.
 */
function & d($name = NULL, $_root_object = FALSE, $allow_creation = TRUE) {
  static $instances = null;
  static $default_instance = '@self';

  if (is_object($name)) {
    return $name;
  }

  if ($name == 'all') {
    return $instances;
  }

  if (is_null($name)) {
    $name = $default_instance;
  }

  if ($_root_object) {
    $default_instance = $name;
  }

  $name = provision_normalise_context_name($name);

  if (isset($instances[$name])) {
    return $instances[$name];
  }
  else {
    $instances[$name] = provision_context_factory($name, $allow_creation);
    if (!is_null($instances[$name])) {
      $instances[$name]->method_invoke('init');
      $instances[$name]->type_invoke('init');
    }
    return $instances[$name];
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

  $name = provision_normalise_context_name($name);

  if (!isset($cache[$name])) {
    $cache[$name] = drush_sitealias_get_record($name);
  }

  return $cache[$name];
}

/**
 * Create a new context object.
 *
 * @param $name
 *   The name of the context object to instantiate.
 * @param $allow_creation
 *   Defaults to TRUE. Allows creating a new context object with the specified
 *   $name.
 *
 * @return
 *   An instance of the specified context, or NULL if it could not be loaded.
 */
function provision_context_factory($name, $allow_creation = TRUE) {
  // the default type, can also be 'platform' or 'site'
  $type = 'server';

  $record = provision_sitealias_get_record($name);
  if (!$allow_creation && empty($record)) {
    drush_set_error('PROVISION_MISSING_CONTEXT', dt('Could not find provision alias named: @name' , array('@name' => $name)));
    return NULL;
  }
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

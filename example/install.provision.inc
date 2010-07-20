<?php
// $Id$

/**
 * @file
 *   Drush invoke API hooks for the 'provision-install' command.
 *
 * This file contains an example of how the developer is intended to 
 * interact with the services defined by the hosting-service API.
 */

/**
 * Implementation of drush_hook_provision_install.
 *
 * Create the site specific configuration for this service.
 *
 * In our basic implementation we do not have an implementation
 * of this API method, because it wasn't necessary.
 *
 * If the service implemented doesn't define this method, or
 * no implementation has been selected, nothing will happen.
 */
function drush_example_provision_install() {
  /**
   * Using the d() accessor.
   *
   * Every object that aegir manages (namely servers, platforms and sites),
   * has an associated 'named context' that we manage for it.
   *
   * You can run the provision commands on different objects, simply
   * by specifying the context name before the drush command, such as:
   *
   *   drush @server_master provision-verify
   *
   * When you have called a command in this way, you can use the d()
   * function without any arguments to retrieve the object representing
   * the current context.
   */
  if (d()->type == 'site') {
    // only run this code on site objects.

    /**
     * Calling service methods.
     *
     * All the provision context objects can register which
     * servers handle specific services for them.
     *
     * To call the correct methods, you just need to use the
     * the 'service' method with the service type you want
     * to call as its only argument.
     */
    d()->service('example')->create_config('site');
  }
}

/**
 * Implementation of drush_hook_provision_install_rollback.
 *
 * All drush invoke hooks allow you to recover if drush_set_error()
 * was set in the previously called hooks.
 *
 * By defining a _rollback variant, you can cleanly handle cases where
 * issues occured, in this case deleting the config file created in the
 * previous step.
 */
function drush_example_provision_install_rollback() {
  d()->service('example')->delete_config('site'); 
}

/**
 * Implementation of drush_hook_post_provision_install.
 *
 * We can now perform such operations as restarting the server,
 * by calling the public API of the service we have created.
 */
function drush_example_post_provision_install() {
  d()->service('example')->parse_configs();
}

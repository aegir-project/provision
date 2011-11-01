<?php

/**
 * The service type base class.
 *
 * All implementations of the service type will inherit this class.
 * This class should define the 'public API' to be used by the rest
 * of the system, which should not expose implementation details.
 */
class Provision_Service_example extends Provision_Service {
  public $service = 'example';

  /**
   * Initialize the service along with the server object.
   */
  function init() {
    // REMEMBER TO CALL THE PARENT!
    parent::init();

    /**
     * We do not need to use this in our example.
     *
     * You would extend this if you needed to save values
     * for all possible implementations of this service type.
     */
  }

  /**
   * Called on provision-verify.
   *
   * We change what we will do based on what the 
   * type of object the command is being run against.
   */
  function verify() {
    $this->create_config(d()->type);
    $this->parse_configs();
  }

  /**
   * PUBLIC API!
   *
   * These are just a basic example, the thing to notice here is that these
   * methods are just stubs for later implementations to extend.
   *
   * If a specific implementation doesn't need to parse the configuration files
   * that are generated for instance, it can just not implement the stub.
   */

  /**
   * Commonly something like running the restart_cmd or sending SIGHUP to a process.
   */
  function parse_configs() {
    return TRUE;
  }

  /**
   * Generate a site specific configuration file
   */
  function create_site_config() {
    return TRUE;
  }

  /**
   * Remove an existing site configuration file.
   */
  function delete_site_config() {
    return TRUE;
  }

  /**
   * Add a new platform specific configuration file.
   */
  function create_platform_config() {
    return TRUE;
  }

  /**
   * Remove an existing platform configuration file.
   */
  function delete_platform_config() {
    return TRUE;
  }

  /**
   * Create a new server specific configuration file.
   */
  function create_server_config() {
    return TRUE;
  }

  /**
   * Remove an existing server specific configuration file
   */
  function delete_server_config() {
    return TRUE;
  }
}

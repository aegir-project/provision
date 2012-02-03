<?php

/**
 * Pack cluster module.
 *
 * This is a rewrite of the cluster module to extend it and support
 * "slave" servers that do not sync files around.
 *
 * It is intended to eventually replace the cluster module, but it is
 * not backwards compatible. Most notably, the "cluster_web_servers"
 * array is here renamed to "master_web_servers".
 */
class Provision_Service_http_pack extends Provision_Service_http {
  static function option_documentation() {
    return array(
      '--slave_web_servers' => 'server with dumbcluster: comma-separated list of slave web servers.'
      '--master_web_server' => 'server with dumbcluster: the master web server.'
    );
  }

  function init_server() {
    $this->server->setProperty('slave_web_servers', array(), TRUE);
    $this->server->setProperty('master_web_server', array(), TRUE);
  }

  /**
   * Run a method on each server in the pack.
   *
   * This function does a logical AND on the return status of each of the
   * methods, and returns TRUE only if they all returned something that
   * can be interpreted as TRUE.
   */
  function _each_server($servers, $method, $args = array()) {
    // Return True by default.
    $ret = TRUE;
    foreach ($servers as $server) {
      // If any methods return false, return false for the whole operation.
      $result = call_user_func_array(array(d($server)->service('http', $this->context), $method), $args);
      $ret = $ret && $result;
    }
    return $ret;
  }

  function parse_configs() {
    $this->_each_server($this->server->master_web_servers, __FUNCTION__);
  }

  function create_config($config) {
    $this->_each_server($this->server->master_web_servers, __FUNCTION__);
  }

  function delete_config($config) { 
    $this->_each_server($this->server->master_web_servers, __FUNCTION__);
  }

  function restart() {
    $this->_each_server($this->server->master_web_servers, __FUNCTION__);
    $this->_each_server($this->server->slave_web_servers, __FUNCTION__);
  }

  /**
   * Support the ability to cloak database credentials using environment variables.
   *
   * The pack supports this functionality only if ALL the servers it maintains
   * supports this functionality.
   */
  function cloaked_db_creds() {
    return $this->_each_server($this->server->master_web_servers, __FUNCTION__) && 
      $this->_each_server($this->server->slave_web_servers, __FUNCTION__);
  }

  function sync() {
    $args = func_get_args();
    $this->_each_server($this->server->master_web_servers, __FUNCTION__, $args);
  }

  function fetch() {
    $args = func_get_args();
    $this->_each_server($this->server->master_web_servers, __FUNCTION__, $args);
  }

  function grant_server_list() {
    return array_merge(
      array_map('d', $this->server->slave_web_servers),
      array($this->context->platform->server)
    );
  }
}

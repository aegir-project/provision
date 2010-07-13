<?php

// $Id$

/**
 * @file
 *    The Apache +SSL service implementation.
 */


class provisionService_http_apache_ssl extends provisionService_http_apache {
  public $ssl_enabled = TRUE;

  class create_server_config() {

  }

}

class provisionConfig_apache_server_ssl extends provisionConfig_apache_server {
  public $template = 'server_ssl.tpl.php';
}


class provisionConfig_apache_site_ssl extends provisionConfig_apache_site {
  public $template = 'vhost_ssl.tpl.php';
  public $description = 'apache ssl configuration file';

  function filename() {
    $this->data['apache_site_conf_path'] . '/' . $this->uri . '_ssl';
  }
}

<?php include('http/nginx/server.tpl.php'); ?>

#######################################################
###  nginx default ssl server
#######################################################

server {
  limit_conn   gulag 10; # like mod_evasive - this allows max 10 simultaneous connections from one IP address
<?php foreach ($server->ip_addresses as $ip) :?>
  listen <?php print $ip . ':' . $http_ssl_port; ?>;
<?php
endforeach;
?>
  server_name  _;
  location / {
     root   /var/www/nginx-default;
     index  index.html index.htm;
  }
}

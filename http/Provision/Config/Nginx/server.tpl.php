# Aegir web server main configuration file

#######################################################
###  nginx.conf main
#######################################################

<?php
$nginx_is_modern = drush_get_option('nginx_is_modern');
if (!$nginx_is_modern && $server->nginx_is_modern) {
  $nginx_is_modern = $server->nginx_is_modern;
}

$nginx_has_gzip = drush_get_option('nginx_has_gzip');
if (!$nginx_has_gzip && $server->nginx_has_gzip) {
  $nginx_has_gzip = $server->nginx_has_gzip;
}

$nginx_config_mode = drush_get_option('nginx_config_mode');
if (!$nginx_config_mode && $server->nginx_config_mode) {
  $nginx_config_mode = $server->nginx_config_mode;
}

if ($nginx_is_modern) {
  print "  limit_conn_zone \$binary_remote_addr zone=gulag:10m;\n";
}
else {
  print "  limit_zone gulag \$binary_remote_addr 10m;\n";
}

if ($nginx_has_gzip) {
  print "  gzip_static       on;\n";
}
?>
<?php if ($nginx_config_mode == 'extended'): ?>
 ## Size Limits
  client_body_buffer_size        64k;
  client_header_buffer_size      32k;
  large_client_header_buffers 32 32k;
  connection_pool_size           256;
  request_pool_size               4k;
  fastcgi_buffer_size           128k;
  fastcgi_buffers             256 4k;
  fastcgi_busy_buffers_size     256k;
  fastcgi_temp_file_write_size  256k;

 ## Timeouts
  client_body_timeout             60;
  client_header_timeout           60;
  send_timeout                    60;
  lingering_time                  30;
  lingering_timeout                5;
  fastcgi_connect_timeout         60;
  fastcgi_send_timeout           300;
  fastcgi_read_timeout           300;

 ## Open File Performance
  open_file_cache max=8000 inactive=30s;
  open_file_cache_valid          60s;
  open_file_cache_min_uses         3;
  open_file_cache_errors          on;

 ## FastCGI Caching
  fastcgi_cache_path /var/lib/nginx/speed
                     levels=2:2:2
                     keys_zone=speed:10m
                     inactive=15m
                     max_size=3g;

 ## General Options
  ignore_invalid_headers          on;
  recursive_error_pages           on;
  reset_timedout_connection       on;
  fastcgi_intercept_errors        on;

 ## Compression
  gzip_buffers      16 8k;
  gzip_comp_level   5;
  gzip_http_version 1.1;
  gzip_min_length   10;
  gzip_types        text/plain text/css application/x-javascript text/xml application/xml application/xml+rss text/javascript;
  gzip_vary         on;
  gzip_proxied      any;

 ## SSL performance
  ssl_session_cache   shared:SSL:10m;
  ssl_session_timeout            10m;
<?php endif; ?>

 ## Default index files
  index         index.php index.html;

 ## Log Format
  log_format        main '"$proxy_add_x_forwarded_for" $host [$time_local] '
                         '"$request" $status $body_bytes_sent '
                         '$request_length $bytes_sent "$http_referer" '
                         '"$http_user_agent" $request_time "$gzip_ratio"';

  client_body_temp_path  /var/lib/nginx/body 1 2;
  access_log             /var/log/nginx/access.log main;
  include                /etc/nginx/fastcgi_params;

<?php print $extra_config; ?>
<?php if ($nginx_config_mode == 'extended'): ?>
#######################################################
###  nginx default maps
#######################################################

###
### Set a cache_uid variable for authenticated users (by @brianmercer and @perusio, fixed by @omega8cc).
###
map $http_cookie $cache_uid {
  default                                        '';
  ~SESS[[:alnum:]]+=(?<session_id>[[:graph:]]+)  $session_id;
}

###
### Live switch of $key_uri for Speed Booster cache depending on $args.
###
map $request_uri $key_uri {
  default                                                                            $request_uri;
  ~(?<no_args_uri>[[:graph:]]+)\?(.*)(utm_|__utm|_campaign|gclid|source=|adv=|req=)  $no_args_uri;
}

###
### Deny all known bots on some URIs.
###
map $http_user_agent $is_bot {
  default                                                    '';
  ~*crawl|goog|yahoo|yandex|spider|bot|tracker|click|parser  is_bot;
}

###
### Deny listed requests for security reasons.
###
map $args $is_denied {
  default                                                                                                      '';
  ~*delete.+from|insert.+into|select.+from|union.+select|onload|\.php.+src|system\(.+|document\.cookie|\;|\.\. is_denied;
}
<?php endif; ?>

#######################################################
###  nginx default server
#######################################################

<?php
$ip_address = !empty($ip_address) ? $ip_address : '*';
?>
server {
  limit_conn   gulag 32; # like mod_evasive - this allows max 32 simultaneous connections from one IP address
<?php
if ($ip_address == '*') {
  print "  listen       {$ip_address}:{$http_port};\n";
}
else {
  foreach ($server->ip_addresses as $ip) {
    print "  listen       {$ip}:{$http_port};\n";
  }
}
?>
  server_name  _;
  location / {
    return 404;
  }
}

#######################################################
###  nginx virtual domains
#######################################################

# virtual hosts
include <?php print $http_pred_path ?>/*;
include <?php print $http_platformd_path ?>/*;
include <?php print $http_vhostd_path ?>/*;
include <?php print $http_postd_path ?>/*;

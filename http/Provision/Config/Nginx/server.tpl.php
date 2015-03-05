# Aegir web server main configuration file

#######################################################
###  nginx.conf main
#######################################################

<?php
$nginx_config_mode = drush_get_option('nginx_config_mode');
if (!$nginx_config_mode && $server->nginx_config_mode) {
  $nginx_config_mode = $server->nginx_config_mode;
}

$nginx_is_modern = drush_get_option('nginx_is_modern');
if (!$nginx_is_modern && $server->nginx_is_modern) {
  $nginx_is_modern = $server->nginx_is_modern;
}

$nginx_has_gzip = drush_get_option('nginx_has_gzip');
if (!$nginx_has_gzip && $server->nginx_has_gzip) {
  $nginx_has_gzip = $server->nginx_has_gzip;
}

$nginx_has_upload_progress = drush_get_option('nginx_has_upload_progress');
if (!$nginx_has_upload_progress && $server->nginx_has_upload_progress) {
  $nginx_has_upload_progress = $server->nginx_has_upload_progress;
}

$satellite_mode = drush_get_option('satellite_mode');
if (!$satellite_mode && $server->satellite_mode) {
  $satellite_mode = $server->satellite_mode;
}

if ($nginx_is_modern) {
  print "  limit_conn_zone \$binary_remote_addr zone=limreq:10m;\n";
}
else {
  print "  limit_zone limreq \$binary_remote_addr 10m;\n";
}

if ($nginx_has_gzip) {
  print "  gzip_static       on;\n";
}

if ($nginx_has_upload_progress) {
  print "  upload_progress uploads 1m;\n";
}
?>

<?php if ($nginx_config_mode == 'extended'): ?>
<?php if ($satellite_mode == 'boa'): ?>
 ## FastCGI params
  fastcgi_param  SCRIPT_FILENAME     $document_root$fastcgi_script_name;
  fastcgi_param  QUERY_STRING        $query_string;
  fastcgi_param  REQUEST_METHOD      $request_method;
  fastcgi_param  CONTENT_TYPE        $content_type;
  fastcgi_param  CONTENT_LENGTH      $content_length;
  fastcgi_param  SCRIPT_NAME         $fastcgi_script_name;
  fastcgi_param  REQUEST_URI         $request_uri;
  fastcgi_param  DOCUMENT_URI        $document_uri;
  fastcgi_param  DOCUMENT_ROOT       $document_root;
  fastcgi_param  SERVER_PROTOCOL     $server_protocol;
  fastcgi_param  GATEWAY_INTERFACE   CGI/1.1;
  fastcgi_param  SERVER_SOFTWARE     ApacheSolarisNginx/$nginx_version;
  fastcgi_param  REMOTE_ADDR         $remote_addr;
  fastcgi_param  REMOTE_PORT         $remote_port;
  fastcgi_param  SERVER_ADDR         $server_addr;
  fastcgi_param  SERVER_PORT         $server_port;
  fastcgi_param  SERVER_NAME         $server_name;
  fastcgi_param  USER_DEVICE         $device;
  fastcgi_param  GEOIP_COUNTRY_CODE  $geoip_country_code;
  fastcgi_param  GEOIP_COUNTRY_CODE3 $geoip_country_code3;
  fastcgi_param  GEOIP_COUNTRY_NAME  $geoip_country_name;
  fastcgi_param  REDIRECT_STATUS     200;
  fastcgi_index  index.php;
<?php endif; ?>

 ## Size Limits
  client_body_buffer_size        64k;
  client_header_buffer_size      32k;
<?php if ($satellite_mode == 'boa'): ?>
  client_max_body_size          100m;
<?php endif; ?>
  connection_pool_size           256;
  fastcgi_buffer_size           128k;
  fastcgi_buffers             256 4k;
  fastcgi_busy_buffers_size     256k;
  fastcgi_temp_file_write_size  256k;
  large_client_header_buffers 32 32k;
<?php if ($satellite_mode == 'boa'): ?>
  map_hash_bucket_size           192;
<?php endif; ?>
  request_pool_size               4k;
  server_names_hash_bucket_size  512;
<?php if ($satellite_mode == 'boa'): ?>
  server_names_hash_max_size    8192;
  types_hash_bucket_size         512;
  variables_hash_max_size       1024;
<?php endif; ?>

 ## Timeouts
  client_body_timeout            180;
  client_header_timeout          180;
  send_timeout                   180;
  lingering_time                  30;
  lingering_timeout                5;
  fastcgi_connect_timeout        10s;
  fastcgi_send_timeout          180s;
  fastcgi_read_timeout          180s;

 ## Open File Performance
  open_file_cache max=8000 inactive=30s;
  open_file_cache_valid          99s;
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
<?php if ($satellite_mode == 'boa'): ?>
  server_tokens                  off;
  fastcgi_hide_header         'Link';
  fastcgi_hide_header  'X-Generator';
  fastcgi_hide_header 'X-Powered-By';
  fastcgi_hide_header 'X-Drupal-Cache';
  fastcgi_hide_header 'X-Drupal-Cache-Tags';
<?php endif; ?>

 ## SSL performance
  ssl_session_cache   shared:SSL:10m;
  ssl_session_timeout            10m;

<?php if ($satellite_mode == 'boa'): ?>
 ## GeoIP support
  geoip_country /usr/share/GeoIP/GeoIP.dat;
<?php endif; ?>

 ## Compression
  gzip_buffers      16 8k;
  gzip_comp_level   8;
  gzip_http_version 1.0;
  gzip_min_length   50;
  gzip_types
    application/atom+xml
    application/javascript
    application/json
    application/rss+xml
    application/vnd.ms-fontobject
    application/x-font-opentype
    application/x-font-ttf
    application/x-javascript
    application/xhtml+xml
    application/xml
    application/xml+rss
    font/opentype
    image/svg+xml
    image/x-icon
    text/css
    text/javascript
    text/plain
    text/xml;
  gzip_vary         on;
  gzip_proxied      any;
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

<?php print $extra_config; ?>
<?php if ($nginx_config_mode == 'extended'): ?>
<?php if ($satellite_mode == 'boa'): ?>
  error_log              /var/log/nginx/error.log crit;
<?php endif; ?>
#######################################################
###  nginx default maps
#######################################################

###
### Support separate Speed Booster caches for various mobile devices.
###
map $http_user_agent $device {
  default                                                                normal;
  ~*Nokia|BlackBerry.+MIDP|240x|320x|Palm|NetFront|Symbian|SonyEricsson  mobile-other;
  ~*iPhone|iPod|Android|BlackBerry.+AppleWebKit                          mobile-smart;
  ~*iPad|Tablet                                                          mobile-tablet;
}

###
### Set a cache_uid variable for authenticated users (by @brianmercer and @perusio, fixed by @omega8cc).
###
map $http_cookie $cache_uid {
  default  '';
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
### Deny crawlers.
###
map $http_user_agent $is_crawler {
  default  '';
  ~*HTTrack|BrokenLinkCheck|2009042316.*Firefox.*3\.0\.10|MJ12|HTMLParser  is_crawler;
  ~*SiteBot|PECL|Automatic|CCBot|BuzzTrack|Sistrix|Offline|Nutch|Mireo     is_crawler;
  ~*SWEB|Morfeus|GSLFbot|HiScan|Riddler|DBot|SEOkicks|PChomebot|Scrap      is_crawler;
  ~*rogerbot                                                               is_crawler;
}

###
### Block semalt botnet.
###
map $http_referer $is_botnet {
  default  '';
  ~*semalt\.com|kambasoft\.com|savetubevideo\.com|bottlenose\.com|yapoga\.com  is_botnet;
  ~*descargar-musica-gratis\.net|baixar-musicas-gratis\.com                    is_botnet;
}

###
### Deny all known bots/spiders on some URIs.
###
map $http_user_agent $is_bot {
  default  '';
  ~*crawl|bot|spider|tracker|click|parser|google|yahoo|yandex|baidu|bing  is_bot;
}

###
### Deny almost all crawlers under high load.
###
map $http_user_agent $deny_on_high_load {
  default  '';
  ~*crawl|spider|tracker|click|parser|google|yahoo|yandex|baidu|bing  deny_on_high_load;
}

###
### Deny listed requests for security reasons.
###
map $args $is_denied {
  default  '';
  ~*delete.+from|insert.+into|select.+from|union.+select|onload|\.php.+src|system\(.+|document\.cookie|\;|\.\. is_denied;
}
<?php endif; ?>

#######################################################
###  nginx default server
#######################################################

server {
  listen       *:<?php print $http_port; ?>;
  server_name  _;
  location / {
<?php if ($satellite_mode == 'boa'): ?>
    expires 99s;
    add_header Cache-Control "public, must-revalidate, proxy-revalidate";
    add_header Access-Control-Allow-Origin *;
    root   /var/www/nginx-default;
    index  index.html index.htm;
<?php else: ?>
    return 404;
<?php endif; ?>
  }
}

<?php if ($satellite_mode == 'boa'): ?>
server {
  listen       *:<?php print $http_port; ?>;
  server_name  127.0.0.1;
  location /nginx_status {
    stub_status on;
    access_log off;
    allow 127.0.0.1;
    deny all;
  }
}
<?php endif; ?>

#######################################################
###  nginx virtual domains
#######################################################

# virtual hosts
include <?php print $http_pred_path ?>/*;
include <?php print $http_platformd_path ?>/*;
include <?php print $http_vhostd_path ?>/*;
include <?php print $http_postd_path ?>/*;

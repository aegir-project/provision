#######################################################
###  nginx.conf site start
#######################################################

  server {
        listen       <?php print $http_port; ?>;
        server_name  <?php print $this->uri; ?> <?php if (!$this->redirection && is_array($this->aliases)) : foreach ($this->aliases as $alias_url) : if (trim($alias_url)) : ?> <?php $alias_url = "." . $alias_url; ?> <?php print $alias_url; ?> <?php endif; endforeach; endif; ?>;
        root         <?php print $this->root; ?>;
        index        index.php index.html;

    ## Deny some crawlers
    if ($http_user_agent ~* (HTTrack|HTMLParser|libwww) ) {
         return 444;
    }
    ## www. redirect
    #  if ($host ~* ^(www\.)(.+)) {
    #    set $rawdomain $2;
    #    rewrite ^/(.*)$  http://$rawdomain/$1 permanent;
    #  }
    ## 6.x starts
    location / {
        try_files $uri @cache;
    }

    location @cache {
        if ( $request_method !~ ^(GET|HEAD)$ ) {
            return 405;
        }
        if ($http_cookie ~ "DRUPAL_UID") {
            return 405;
        }
        error_page 405 = @drupal;
        add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";        
        add_header Cache-Control "must-revalidate, post-check=0, pre-check=0";
        add_header X-Header "Boost Citrus 1.9";               
        charset utf-8;
        try_files /cache/normal/$host${uri}_$args.html @drupal;
    }

    location @drupal {
        ###
        ### now simplified to reduce rewrites
        ###
        rewrite ^/(.*)$  /index.php?q=$1 last;
    }

    location ~* (/\..*|settings\.php$|\.(htaccess|engine|inc|info|install|module|profile|pl|po|sh|.*sql|theme|tpl(\.php)?|xtmpl)$|^(Entries.*|Repository|Root|Tag|Template))$ {
        deny all;
    }

    location ~* /(files|themes|sites)/.*\.php$ {
        return 444;                     ### deny php here
    }
    location ~* ^/sites/(.*)/files/backup_migrate/ {
        return 444;                     ### deny direct access
    }    
       
    location ~ \.php$ {
          try_files $uri @drupal;       ### check for existence of php file
          fastcgi_pass 127.0.0.1:9000;  ### php-fpm listening on port 9000
          fastcgi_index index.php;
          fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
    }

    location ~ \.css$ {
        if ( $request_method !~ ^(GET|HEAD)$ ) {
            return 405;
        }
        if ($http_cookie ~ "DRUPAL_UID") {
            return 405;
        }
        error_page 405 = @uncached;
        access_log  off;
        expires  max; #if using aggregator
        add_header X-Header "Boost Citrus 2.1";
        try_files /cache/perm/$host${uri}_.css $uri =404;
    }
    
    location ~ \.js$ {
        if ( $request_method !~ ^(GET|HEAD)$ ) {
            return 405;
        }
        if ($http_cookie ~ "DRUPAL_UID") {
            return 405;
        }
        error_page 405 = @uncached;
        access_log  off;
        expires  max; # if using aggregator
        add_header X-Header "Boost Citrus 2.2";               
        try_files /cache/perm/$host${uri}_.js $uri =404;
    }

    location ~ \.json$ {
        if ( $request_method !~ ^(GET|HEAD)$ ) {
            return 405;
        }
        if ($http_cookie ~ "DRUPAL_UID") {
            return 405;
        }
        error_page 405 = @uncached;
        access_log  off;
        expires  max; ### if using aggregator
        add_header X-Header "Boost Citrus 2.3";               
        try_files /cache/normal/$host${uri}_.json $uri =404;
    }

    location @uncached {
        access_log  off;
        expires  max; # max if using aggregator, otherwise sane expire time
    }

    location ~* /(files/imagecache)|(fckeditor)|(ckeditor)/ {
        access_log         off;
        expires            30d;
        try_files $uri @drupal; ### imagecache and (f)ckeditor support
    }

    location ~* ^.+\.(jpg|jpeg|gif|png|ico|swf)$ {
        access_log      off;
        expires         30d;
        try_files $uri =404;
    }

    location ~* \.xml$ {
        if ( $request_method !~ ^(GET|HEAD)$ ) {
            return 405;
        }
        if ($http_cookie ~ "DRUPAL_UID") {
            return 405;
        }
        error_page 405 = @drupal;
        add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";
        add_header Cache-Control "must-revalidate, post-check=0, pre-check=0";
        add_header X-Header "Boost Citrus 2.4";               
        charset utf-8;
        types { }
        default_type application/rss+xml;
        try_files /cache/normal/$host${uri}_.xml /cache/normal/$host${uri}_.html $uri @drupal;
    }

    location ~* /feed$ {
        if ( $request_method !~ ^(GET|HEAD)$ ) {
            return 405;
        }
        if ($http_cookie ~ "DRUPAL_UID") {
            return 405;
        }
        error_page 405 = @drupal;
        add_header Expires "Tue, 24 Jan 1984 08:00:00 GMT";
        add_header Cache-Control "must-revalidate, post-check=0, pre-check=0";
        add_header X-Header "Boost Citrus 2.5";               
        charset utf-8;
        types { }
        default_type application/rss+xml;
        try_files /cache/normal/$host${uri}_.xml /cache/normal/$host${uri}_.html $uri @drupal;
    }

  } # end of server

#######################################################
###  nginx.conf site end
#######################################################

<?php
if ($this->redirection) {
  //require('/http/nginx/vhost_redirect.tpl.php');
}


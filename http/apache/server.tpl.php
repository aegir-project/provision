# Aegir web server configuration file

NameVirtualHost *:<?php print $http_port; ?>


<VirtualHost *:<?php print $http_port; ?>>
  ServerName default
  Redirect 404 /
</VirtualHost>


<IfModule !env_module>
  LoadModule env_module modules/mod_env.so
</IfModule>

<IfModule !rewrite_module>
  LoadModule rewrite_module modules/mod_rewrite.so
</IfModule>

# virtual hosts
Include <?php print $http_vhostd_path ?>

# platforms
Include <?php print $http_platformd_path ?>

# other configuration, not touched by aegir
Include <?php print $http_confd_path ?>


<?php print $extra_config; ?>

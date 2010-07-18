# Aegir web server configuration file

<?php foreach ($server->ip_addresses as $ip) : ?>
  Listen <?php print $ip . ':' . $http_port; ?>

  NameVirtualHost <?php print $ip . ':' . $http_port; ?>

<?php endforeach; ?>

<VirtualHost <?php print $ip_address . ':' . $http_port; ?>>
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

# Aegir web server configuration file

<?php if (is_array($web_ports)) :
  foreach ($web_ports as $web_port) :?>
  NameVirtualHost *:<?php print $web_port; ?>

  <VirtualHost *:<?php print $web_port; ?>>
    ServerName default
    Redirect 404 /
  </VirtualHost>
  
<?php
endforeach;
endif;
?>

<IfModule !env_module>
  LoadModule env_module modules/mod_env.so
</IfModule>

<IfModule !rewrite_module>
  LoadModule rewrite_module modules/mod_rewrite.so
</IfModule>

# virtual hosts
Include <?php print $config_path ?>/vhost.d/
# platforms
Include <?php print $config_path ?>/platform.d/
# other configuration, not touched by aegir
Include <?php print $config_path ?>/apache.d/


<?php print $extra_config; ?>

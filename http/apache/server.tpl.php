# Aegir web server configuration file

<?php if (is_array(d()->web_ports)) :
  foreach (d()->web_ports as $web_port) :?>
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
Include <?php print $this->apache_site_conf_path ?>

# platforms
Include <?php print $this->apache_platform_conf_path ?>

# other configuration, not touched by aegir
Include <?php print $this->apache_conf_path ?>


<?php print $extra_config; ?>

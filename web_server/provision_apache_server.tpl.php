# Aegir web server configuration file

<?php if (is_array($web_ports)) :
  foreach ($web_ports as $web_port) :?>
  NameVirtualHost *:<?php print $web_port; ?>
  
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

Include <?php print $config_path ?>/vhost.d/
Include <?php print $config_path ?>/platform.d/

<?php print $extra_config; ?>

# Aegir web server configuration file

<?php if (is_array($web_ports)) :
  foreach ($web_ports as $web_port) :?>
  NameVirtualHost *:<?php print $web_port; ?>
  
<?php
endforeach;
endif;
?>

<?php print $extra_config; ?>

<VirtualHost <?php print $ip_address . ':' . $http_port; ?>>
<?php if ($this->site_mail) : ?>
  ServerAdmin <?php  print $this->site_mail; ?> 
<?php endif;?>

  DocumentRoot <?php print $this->root; ?> 
    
  ServerName <?php print $this->uri; ?>

  SetEnv db_type  <?php print urlencode($db_type); ?>

  SetEnv db_name  <?php print urlencode($db_name); ?>

  SetEnv db_user  <?php print urlencode($db_user); ?>

  SetEnv db_passwd  <?php print urlencode($db_passwd); ?>

  SetEnv db_host  <?php print urlencode($db_host); ?>

  SetEnv db_port  <?php print urlencode($db_port); ?>

<?php if (!$this->redirection && is_array($this->aliases)) :
  foreach ($this->aliases as $alias_url) :
  if (trim($alias_url)) : ?>
  ServerAlias <?php print $alias_url; ?> 

<?php
 endif;
 endforeach;
 endif; ?>

<?php print $extra_config; ?>

    # Error handler for Drupal > 4.6.7
    <Directory "<?php print $this->site_path; ?>/files">
      SetHandler This_is_a_Drupal_security_line_do_not_remove
    </Directory>

</VirtualHost>

<?php
if ($this->redirection) {
//  require(dirname(__FILE__) . '/vhost_redirect.tpl.php');
}

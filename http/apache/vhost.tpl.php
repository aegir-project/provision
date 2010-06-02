<VirtualHost *:<?php print $site_port; ?>>
<?php if ($site_mail) : ?>
  ServerAdmin <?php  print $site_mail; ?> 
<?php endif;?>
  DocumentRoot <?php print $this->platform->root; ?> 
    
  ServerName <?php print $site_url; ?>

  SetEnv db_type  <?php print urlencode($db_type); ?>

  SetEnv db_name  <?php print urlencode($db_name); ?>

  SetEnv db_user  <?php print urlencode($db_user); ?>

  SetEnv db_passwd  <?php print urlencode($db_passwd); ?>

  SetEnv db_host  <?php print urlencode($db_host); ?>

<?php if (!$redirection && is_array($aliases)) :
  foreach ($aliases as $alias_url) :
  if (trim($alias_url)) : ?>
  ServerAlias <?php print $alias_url; ?> 

<?php
 endif;
 endforeach;
 endif; ?>

<?php print $extra_config; ?>

    # Error handler for Drupal > 4.6.7
    <Directory "<?php print $this->platform->root; ?>/sites/<?php print trim($site_url, '/'); ?>/files">
      SetHandler This_is_a_Drupal_security_line_do_not_remove
    </Directory>

</VirtualHost>

<?php
if ($redirection) {
  include('vhost_redirect.tpl.php');
}

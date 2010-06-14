<Directory <?php print $this->root; ?>>
    Order allow,deny
    Allow from all
<?php print $extra_config; ?>
</Directory>


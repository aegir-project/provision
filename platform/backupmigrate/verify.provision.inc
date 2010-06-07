<?php

/**
 * Inject the relevant .htacces configuration into the global apache configuration
 */
function provision_backupmigrate_provision_apache_dir_config($data = null) {
  return <<<EOF
<IfModule mod_rewrite.c>
  RewriteEngine on
  RewriteRule sites/%{SERVER_NAME}/files/backup_migrate - [F]
  RewriteRule files/backup_migrate - [F]
</IfModule>
EOF;
}

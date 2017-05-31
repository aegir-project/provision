<?php
/**
 * @file
 * Provides the Provision_Config_Drupal_Settings class.
 */

class Provision_Config_Drupal_Settings extends Provision_Config {
  public $template = 'provision_drupal_settings_7.tpl.php';
  public $description = 'Drupal settings.php file';
  public $creds = array();
  protected $mode = 0440;

  function filename() {
    return $this->site_path . '/settings.php';
  }

  function process() {
    if (drush_drupal_major_version() >= 8) {
      $this->template = 'provision_drupal_settings_8.tpl.php';
      $this->data['db_type'] = ($this->data['db_type'] == 'mysqli') ? 'mysql' : $this->data['db_type'];
      $this->data['utf8mb4_is_configurable'] = TRUE;
      $this->data['utf8mb4_is_supported'] = $this->db_server->utf8mb4_is_supported;
      $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
      require_once $drupal_root . '/core/lib/Drupal/Component/Utility/Crypt.php';
      $this->data['drupal_hash_salt_var'] = Drupal\Component\Utility\Crypt::randomBytesBase64(55);
      $this->data['maintenance_var_new'] = TRUE;
    }
    elseif (drush_drupal_major_version() == 7) {
      $this->template = 'provision_drupal_settings_7.tpl.php';
      $this->data['db_type'] = ($this->data['db_type'] == 'mysqli') ? 'mysql' : $this->data['db_type'];
      $this->data['utf8mb4_is_configurable'] = version_compare(drush_drupal_version(), '7.50', '>=');
      $this->data['utf8mb4_is_supported'] = $this->db_server->utf8mb4_is_supported;
      $this->data['maintenance_var_new'] = TRUE;
    }
    elseif (drush_drupal_major_version() <= 6) {
      $this->template = 'provision_drupal_settings_6.tpl.php';
      $this->data['maintenance_var_new'] = FALSE;
    }

    $this->version = provision_version();
    $this->api_version = provision_api_version();
    $this->cloaked = drush_get_option('provision_db_cloaking', $this->context->service('http')->cloaked_db_creds());

    if (provision_hosting_feature_enabled('subdirs')) {
      $this->data['subdirs_support_enabled'] = TRUE;
    }
    else {
      $this->data['subdirs_support_enabled'] = drush_get_option('subdirs_support');
    }

    foreach (array('db_type', 'db_user', 'db_passwd', 'db_host', 'db_name', 'db_port') as $key) {
      $this->creds[$key] = urldecode($this->data[$key]);
    }

    $this->data['extra_config'] = "# Extra configuration from modules:\n";
    $this->data['extra_config'] .= join("\n", drush_command_invoke_all('provision_drupal_config', d()->uri, $this->data));

    $this->group = $this->platform->server->web_group;

    // Add a handy variable indicating if the site is being backed up, we can
    // then react to this and change any settings we don't want backed up.
    $backup_file = drush_get_option('backup_file');
    $this->backup_in_progress = !empty($backup_file);

    // Create a blank local.settings.php file if not exists and option calls for this.
    $local_settings = $this->site_path . '/local.settings.php';
    $local_settings_blank = "<?php # local settings.php \n";
    $local_description = 'Drupal local.settings.php file';
    if (!provision_file()->exists($local_settings)->status() &&
        drush_get_option('provision_create_local_settings_file', TRUE)) {
      provision_file()->file_put_contents($local_settings, $local_settings_blank)
        ->succeed('Generated blank ' . $local_description)
        ->fail('Could not generate ' . $local_description);
    }

    // Set permissions on local.settings.php, if it exists.
    if (provision_file()->exists($local_settings)->status()) {
      provision_file()->chgrp($local_settings, $this->group)
        ->succeed('Changed group ownership of <code>@path</code> to @gid')
        ->fail('Could not change group ownership of <code>@path</code> to @gid');
      provision_file()->chmod($local_settings, $this->mode | 0440)
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions of <code>@path</code> to @perm');
    }
  }
}

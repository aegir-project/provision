<?php
/**
 * @file
 * Provides the Provision_Config_Drupal_Settings class.
 */

class Provision_Config_Drupal_Settings extends Provision_Config {
  public $template = 'provision_drupal_settings.tpl.php';
  public $description = 'Drupal settings.php file';
  public $creds = array();
  protected $mode = 0440;

  function filename() {
    return $this->site_path . '/settings.php';
  }

  function process() {
    if (drush_drupal_major_version() >= 7) {
      $this->data['db_type'] = ($this->data['db_type'] == 'mysqli') ? 'mysql' : $this->data['db_type'];
      $this->data['file_directory_path_var'] = 'file_public_path';
      $this->data['file_directory_temp_var'] = 'file_temporary_path';
      $this->data['file_directory_private_var'] = 'file_private_path';
      $this->data['drupal_hash_salt_var'] = 'empty';
    }
    else {
      $this->data['file_directory_path_var'] = 'file_directory_path';
      $this->data['file_directory_temp_var'] = 'file_directory_temp';
    }
    if (drush_drupal_major_version() >= 8) {
      $this->template = 'provision_drupal_settings_8.tpl.php';

      $drupal_root = drush_get_context('DRUSH_DRUPAL_ROOT');
      require_once $drupal_root . '/core/lib/Drupal/Component/Utility/Crypt.php';
      $this->data['drupal_hash_salt_var'] = Drupal\Component\Utility\Crypt::randomBytesBase64(55);

      $this->data['config_directories_active_var'] = 'config_directories_active';
      $this->data['config_directories_staging_var'] = 'config_directories_staging';
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

    // Create a blank local.settings.php file if not exists.
    $local_settings = $this->site_path . '/local.settings.php';
    $local_settings_blank = "<?php # local settings.php \n";
    $local_description = 'Drupal local.settings.php file';
    if (!provision_file()->exists($local_settings)->status()) {
      provision_file()->file_put_contents($local_settings, $local_settings_blank)
        ->succeed('Generated blank ' . $local_description)
        ->fail('Could not generate ' . $local_description);
      provision_file()->chgrp($local_settings, $this->group)
        ->succeed('Changed group ownership of <code>@path</code> to @gid')
        ->fail('Could not change group ownership of <code>@path</code> to @gid');
      provision_file()->chmod($local_settings, $this->mode | 0440)
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions of <code>@path</code> to @perm');
    }
    else {
      provision_file()->chgrp($local_settings, $this->group)
        ->succeed('Changed group ownership of <code>@path</code> to @gid')
        ->fail('Could not change group ownership of <code>@path</code> to @gid');
      provision_file()->chmod($local_settings, $this->mode | 0440)
        ->succeed('Changed permissions of <code>@path</code> to @perm')
        ->fail('Could not change permissions of <code>@path</code> to @perm');
    }
  }
}

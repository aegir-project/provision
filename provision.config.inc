<?php

// $Id$

/**
 * Provision configuration generation classes.
 */

class provisionConfig {
  public $template = null;
  public $data = array();
  public $description = null;
  protected $append = FALSE;
  protected $mode = NULL;
  protected $group = NULL;

  function __construct($data = array()) {
    if (is_null($this->template)) {
      throw(exception);
    }
    if (sizeof($data)) {
      $this->data = $data;
    }
  }

  function process() {
    parent::process();
    return true;
  }

  function filename() {
    return false;
  }


  private function load_template() {
    $reflect = new reflectionObject($this);
    $base_dir = dirname($reflect->getFilename());

    if (isset($this->template)) {
      $file = $base_dir . '/' . $this->template;
      drush_log("Template loaded: $file");
      if (file_exists($file) && is_readable($file)) {
        return file_get_contents($file);
      }
    }
    return false;
  }


  private function render_template($template, $variables) {
    drush_errors_off();
    extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
    ob_start();                      // Start output buffering
    eval('?>'. $template);                 // Generate content
    $contents = ob_get_contents();   // Get the contents of the buffer
    ob_end_clean();                  // End buffering and discard
    drush_errors_on();
    return $contents;                // Return the contents
  }

  function write() {
    $filename = $this->filename();
    if ($filename && is_writeable(dirname($filename))) {
      // manipulate data before passing to template.
      $this->process();

      if ($template = $this->load_template()) {
        // Make sure we can write to the file
        if (!is_null($this->mode) && !($this->mode & 0200) && provision_service('file')->exists($filename)->status()) {
          provision_service('file')->chmod($filename, $this->mode | 0200)
            ->succeed('Changed permissions of @path to @perm')
            ->fail('Could not change permissions of @path to @perm');
        }

        provision_service('file')->file_put_contents($filename, $this->render_template($template, $this->data), $this->append ? FILE_APPEND : 0)
          ->succeed('Generated config ' . (empty($this->description) ? $filename : $this->description), 'success');

        // Change the permissions of the file if needed
        if (!is_null($this->mode)) {
          provision_service('file')->chmod($filename, $this->mode)
            ->succeed('Changed permissions of @path to @perm')
            ->fail('Could not change permissions of @path to @perm');
        }
        if (!is_null($this->group)) {
          provision_service('file')->chgrp($filename, $this->group)
            ->succeed('Change group ownership of @path to @gid')
            ->fail('Could not change group ownership of @path to @gid');
        }
      }
    }
  }

  function unlink() {
    return provision_service('file')->unlink($this->filename())->status();
  }
}

/**
 * Specialized class to handle the creation of drushrc.php files.
 *
 * This is based on the drush_save_config code, but has been abstracted
 * for our purposes.
 */ 
class provisionConfig_drushrc extends provisionConfig {
  public $template = 'provision_drushrc.tpl.php';
  public $description = 'Drush configuration file';
  protected $mode = 0400;
  protected $context_name = 'drush';

  function filename() {
    return _drush_config_file($this->context_name);
  }

  function __construct($data = array()) {
    parent::__construct($data);
    $this->load_data();

  }
  function load_data() {
    // we fetch the context to pass into the template based on the context name
    $this->data = array_merge(drush_get_context($this->context_name), $this->data);
  }

  function process() {
    unset($this->data['context-path']);
    unset($this->data['config-file']);
    $this->data['option_keys'] = array_keys($this->data);
  }
}

// class to read and write an alias record
class provisionConfig_drushrc_alias extends provisionConfig_drushrc {
  function __construct($aliasname, $data = array()) {
    parent::construct($data);
    $this->alias_name = $aliasname;
  }

  function load_data() {
     // do something about alias_load_data
  }

  function filename() {
    return drush_get_option('alias-path', drush_server_home() . '/.drush/aliases/') . trim($this->aliasname, '@'); 
  }
}


/**
 * Server level config for drushrc.php files.
 */
class provisionConfig_drushrc_server extends provisionconfig_drushrc {
  protected $context_name = 'user';
  public $description = 'Server drush configuration';
}

/**
 * Class for writing $platform/drushrc.php files.
 */
class provisionConfig_drushrc_platform extends provisionConfig_drushrc {
  protected $context_name = 'drupal';
  public $description = 'Platform Drush configuration file';
}

/**
 * Class for writing $platform/sites/$url/drushrc.php files.
 */
class provisionConfig_drushrc_site extends provisionConfig_drushrc {
  protected $context_name = 'site';
  public $template = 'provision_drushrc_site.tpl.php';
  public $description = 'Site Drush configuration file';
}


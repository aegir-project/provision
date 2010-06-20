<?php

// $Id$

/**
 * Provision configuration generation classes.
 */

class provisionConfig {
  /**
   * Template file, a PHP file which will have access to $this and variables
   * as defined in $data.
   */
  public $template = null;

  /**
   * Associate array of variables to make available to the template.
   */
  public $data = array();

  /**
   * A provisionEnvironment object thie configuration relates to.
   *
   * @var provisionEnvironment
   */
  public $owner = null;

  /**
   * If set, replaces file name in log messages.
   */
  public $description = null;

  /**
   * Octal Unix mode for permissons of the created file.
   */
  protected $mode = NULL;

  /**
   * Unix group name for the created file.
   */
  protected $group = NULL;

  /**
   * Forward $this->... to $this->owner->...
   * object.
   */
  function __get($name) {
    if (isset($this->owner)) {
      return $this->owner->$name;
    }
  }

  /**
   * Constructor, overriding not recommended.
   *
   * @param $owner
   *   An alias name for d(), the provisionEnvironment that this configuration
   *   is relevant to.
   * @param $data
   *   An associative array to potentiall manipulate in process() and make
   *   available as variables to the template.
   */
  function __construct($owner, $data = array()) {
    if (is_null($this->template)) {
      throw(exception);
    }
    $this->data = $this->parse();
    if (sizeof($data)) {
      $this->data = array_merge($this->data, $data);
    }
    $this->owner = d($owner);
  }

  /**
   * Parse the existing config to load data
   *
   * This is called in the constructor, and should take care of
   * opening the file and parsing it into the data array.
   */
  function parse() {
    return array();
  }

  /**
   * Process and add to $data before writing the configuration.
   *
   * This is a stub to be implemented by subclasses.
   */
  function process() {
    return true;
  }

  /**
   * The filename where the configuration is written.
   * 
   * This is a stub to be implemented by subclasses.
   */
  function filename() {
    return false;
  }

  /**
   * Load template from filename().
   */
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

  /**
   * Render template, making variables available from $variables associative
   * array.
   */
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

  /**
   * Write out this configuration.
   *
   * 1. Make sure parent directory exists and is writable.
   * 2. Load template with load_template().
   * 3. Process $data with process().
   * 4. Make existing file writable if necessary and possible.
   * 5. Render template with $this and $data and write out to filename().
   * 6. If $mode and/or $group are set, apply them for the new file.
   */
  function write() {
    $filename = $this->filename();
    // Make directory structure if it does not exist.
    if (!provision_file()->exists(dirname($filename))->status()) {
      provision_file()->mkdir(dirname($filename))
        ->succeed('Created directory @path.')
        ->fail('Could not create directory @path.');
    }

    if ($filename && is_writeable(dirname($filename))) {
      // manipulate data before passing to template.
      $this->process();

      if ($template = $this->load_template()) {
        // Make sure we can write to the file
        if (!is_null($this->mode) && !($this->mode & 0200) && provision_file()->exists($filename)->status()) {
          provision_file()->chmod($filename, $this->mode | 0200)
            ->succeed('Changed permissions of @path to @perm')
            ->fail('Could not change permissions of @path to @perm');
        }

        provision_file()->file_put_contents($filename, $this->render_template($template, $this->data))
          ->succeed('Generated config ' . (empty($this->description) ? $filename : $this->description), 'success');

        // Change the permissions of the file if needed
        if (!is_null($this->mode)) {
          provision_file()->chmod($filename, $this->mode)
            ->succeed('Changed permissions of @path to @perm')
            ->fail('Could not change permissions of @path to @perm');
        }
        if (!is_null($this->group)) {
          provision_file()->chgrp($filename, $this->group)
            ->succeed('Change group ownership of @path to @gid')
            ->fail('Could not change group ownership of @path to @gid');
        }
      }
    }
  }

  /**
   * Remove configuration file as specified by filename().
   */
  function unlink() {
    provision_file()->unlink($this->filename())->status();
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

  function __construct($owner, $data = array()) {
    parent::__construct($owner, $data);
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

/**
 * Class to write an alias records.
 */
class provisionConfig_drushrc_alias extends provisionConfig_drushrc {
  public $template = 'provision_drushrc_alias.tpl.php';

  /**
   * @param $name
   *   String '\@name' for named context.
   * @param $options
   *   Array of string option names to save.
   */
  function __construct($owner, $data = array()) {
    parent::__construct($owner, $data);
    $this->data = array(
      'aliasname' => ltrim($owner, '@'),
      'options' => $data,
    );
  }

  function filename() {
    return drush_server_home() . '/.drush/' . $this->data['aliasname'] . '.alias.drushrc.php'; 
  }
}

/**
 * Server level config for drushrc.php files.
 */
class provisionConfig_drushrc_server extends provisionConfig_drushrc {
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


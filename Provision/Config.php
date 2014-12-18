<?php
/**
 * @file
 * Provision configuration generation classes.
 */

class Provision_Config {
  /**
   * Template file, a PHP file which will have access to $this and variables
   * as defined in $data.
   */
  public $template = NULL;

  /**
   * Associate array of variables to make available to the template.
   */
  public $data = array();

  /**
   * A Provision_Context object thie configuration relates to.
   *
   * @var Provision_Context
   */
  public $context = NULL;

  /**
   * If set, replaces file name in log messages.
   */
  public $description = NULL;

  /**
   * Octal Unix mode for permissons of the created file.
   */
  protected $mode = NULL;

  /**
   * Unix group name for the created file.
   */
  protected $group = NULL;

  /**
   * An optional data store class to instantiate for this config.
   */
  protected $data_store_class = NULL;

  /**
   * The data store.
   */
  public $store = NULL;

  /**
   * Forward $this->... to $this->context->...
   * object.
   */
  function __get($name) {
    if (isset($this->context)) {
      return $this->context->$name;
    }
  }

  /**
   * Constructor, overriding not recommended.
   *
   * @param $context
   *   An alias name for d(), the Provision_Context that this configuration
   *   is relevant to.
   * @param $data
   *   An associative array to potentially manipulate in process() and make
   *   available as variables to the template.
   */
  function __construct($context, $data = array()) {
    if (is_null($this->template)) {
      throw new Exception(dt("No template specified for: %class", array('%class' => get_class($this))));
    }

    // Accept both a reference and an alias name for the context.
    $this->context = is_object($context) ? $context : d($context);

    if (sizeof($data)) {
      $this->data = $data;
    }

    if (!is_null($this->data_store_class) && class_exists($this->data_store_class)) {
      $class = $this->data_store_class;
      $this->store = new $class($context, $data);
    }

  }

  /**
   * Process and add to $data before writing the configuration.
   *
   * This is a stub to be implemented by subclasses.
   */
  function process() {
    if (is_object($this->store)) {
      $this->data['records'] = array_filter(array_merge($this->store->loaded_records, $this->store->records));
    }
    return TRUE;
  }

  /**
   * The filename where the configuration is written.
   *
   * This is a stub to be implemented by subclasses.
   */
  function filename() {
    return FALSE;
  }

  /**
   * Load template from filename().
   *
   * @see hook_provision_config_load_templates()
   * @see hook_provision_config_load_templates_alter()
   */
  private function load_template() {

    // Allow other Drush commands to change the template used first.
    $templates = drush_command_invoke_all('provision_config_load_templates', $this);
    // Ensure that templates is at least an array.
    if (!is_array($templates)) {
      $templates = array();
    }
    // Allow other Drush commands to alter the templates from other commands.
    drush_command_invoke_all_ref('provision_config_load_templates_alter', $templates, $this);
    if (!empty($templates) && is_array($templates)) {
      foreach ($templates as $file) {
        if (is_readable($file)) {
          drush_log(dt('Template loaded from hook(s): :file'), array(
            ':file' => $file,
          ));
          return file_get_contents($file);
        }
      }
    }

    // If we've got this far, then try to find a template from this class or
    // one of its parents.
    if (isset($this->template)) {
      $class_name = get_class($this);
      while ($class_name) {
        // Iterate through the config file's parent classes until we
        // find the template file to use.
        $base_dir = provision_class_directory($class_name);
        $file = $base_dir . '/' . $this->template;

        if (is_readable($file)) {
          drush_log(dt('Template loaded from Provision Config class :class_name: :file', array(
            ':class_name' => $class_name,
            ':file' => $file,
          )));
          return file_get_contents($file);
        }

        $class_name = get_parent_class($class_name);
      }
    }

    // We've failed to find a template if we've reached this far.
    drush_log(dt('No template found for Provision Config class: ', array(':class' => get_class($this))), 'warning');
    return FALSE;
  }

  /**
   * Render template, making variables available from $variables associative
   * array.
   */
  private function render_template($template, $variables) {
    drush_errors_off();
    extract($variables, EXTR_SKIP);  // Extract the variables to a local namespace
    ob_start();                      // Start output buffering
    eval('?>' . $template);                 // Generate content
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
    if ($filename && !provision_file()->exists(dirname($filename))->status()) {
      provision_file()->mkdir(dirname($filename))
        ->succeed('Created directory @path.')
        ->fail('Could not create directory @path.');
    }

    $status = FALSE;
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

        $status = provision_file()->file_put_contents($filename, $this->render_template($template, $this->data))
          ->succeed('Generated config in write(): ' . (empty($this->description) ? $filename : $this->description . ' (' . $filename. ')'), 'success')
          ->fail('Could not generate in write(): ' . (empty($this->description) ? $filename : $this->description . ' (' . $filename. ')'))->status();

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
    return $status;
  }

  // allow overriding w.r.t locking
  function file_put_contents($filename, $text) {
    provision_file()->file_put_contents($filename, $text)
      ->succeed('Generated config in file_put_contents()' . (empty($this->description) ? $filename : $this->description), 'success');
  }

  /**
   * Remove configuration file as specified by filename().
   */
  function unlink() {
    return provision_file()->unlink($this->filename())->status();
  }

}

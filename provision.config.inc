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
   * A provisionContext object thie configuration relates to.
   *
   * @var provisionContext
   */
  public $context = null;

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
   *   An alias name for d(), the provisionContext that this configuration
   *   is relevant to.
   * @param $data
   *   An associative array to potentially manipulate in process() and make
   *   available as variables to the template.
   */
  function __construct($context, $data = array()) {
    if (is_null($this->template)) {
      throw(exception);
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
    $class_name = get_class($this);

    $reflect = new reflectionObject($this);

    if (isset($this->template)) {
      while ($class_name) {
        // Iterate through the config file's parent classes until we
        // find the template file to use.
        $reflect = new reflectionClass($class_name);
        $base_dir = dirname($reflect->getFilename());

        $file = $base_dir . '/' . $this->template;

        if (file_exists($file) && is_readable($file)) {
          drush_log("Template loaded: $file");
          return file_get_contents($file);
        }

        $class_name = get_parent_class($class_name);
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
          ->succeed('Generated config ' . (empty($this->description) ? $filename : $this->description), 'success')
          ->fail('Could not generate ' . (empty($this->description) ? $filename : $this->description))->status();

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
      ->succeed('Generated config ' . (empty($this->description) ? $filename : $this->description), 'success');
  }

  /**
   * Remove configuration file as specified by filename().
   */
  function unlink() {
    return provision_file()->unlink($this->filename())->status();
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

  function __construct($context, $data = array()) {
    parent::__construct($context, $data);
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
  function __construct($context, $data = array()) {
    parent::__construct($context, $data);
    $this->data = array(
      'aliasname' => ltrim($context, '@'),
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
  // platforms contain no confidential information
  protected $mode = 0444;

  function filename() {
    return $this->root . '/drushrc.php';
  }
}

/**
 * Class for writing $platform/sites/$url/drushrc.php files.
 */
class provisionConfig_drushrc_site extends provisionConfig_drushrc {
  protected $context_name = 'site';
  public $template = 'provision_drushrc_site.tpl.php';
  public $description = 'Site Drush configuration file';

  function filename() {
    return $this->site_path . '/drushrc.php';
  }
}

/**
 * Base class for data storage.
 *
 * This class provides a file locking mechanism for configuration
 * files that may be susceptible to race conditions.
 *
 * The records loaded from the config and the records set in this
 * instance are kept in separate arrays.
 *
 * When we lock the file, we load the latest stored info.
 */
class provisionConfig_data_store extends provisionConfig {
  public $template = 'data_store.tpl.php';
  public $key = 'record';

  private $locked = FALSE;
  protected $fp = null;

  public $records = array();
  public $loaded_records = array();

  protected $mode = 0700;


  function __construct($context, $data = array()) {
    parent::__construct($context, $data);

    $this->load_data();
  }

  /**
   * Ensure the file pointer is closed and the lock released upon destruction.
   */
  function __destruct() {
    // release the file lock if we have it.
    $this->close();
  }

  /**
   * Open the file.
   */
  function open() {
    if (!is_resource($this->fp)) {
      $this->fp = fopen($this->filename(), "w+");
    }
  }

  /**
   * Lock the file from other writes.
   *
   * After the file has been locked, we reload the data from the file
   * so that any changes we make will not override previous changes.
   */
  function lock() {
    if (!$this->locked) {
      $this->open();
      flock($this->fp, LOCK_EX);

      // Do one last load before setting our locked status.
      $this->load_data();
      $this->locked = TRUE;
    }
  }

  /**
   * Put the contents in the locked file.
   *
   * We call the lock method here to insure we have the lock.
   */
  function file_put_contents($filename, $text) {
    $this->lock();
    fwrite($this->fp, $text);
    fflush($this->fp);
  }

  /**
   * Release the write log on the data store file.
   */
  function unlock() {
    if ($this->locked && is_resource($this->fp)) {
      flock($this->fp, LOCK_UN);
      $this->locked = FALSE;
    }
  }

  /**
   * Close the file pointer and release the lock (if applicable).
   */
  function close() {
    if (is_resource($this->fp)) {
      fclose($this->fp);
    }
  }

  /**
   * Load the data from the data store into our loaded_records property.
   */
  function load_data() {
    if (!$this->locked) {
      // Once we have the lock we dont need to worry about it changing
      // from under us.
      if (file_exists($this->filename()) && is_readable($this->filename())) {
        include($this->filename());
        $data_key = $this->key;
        if (isset(${$data_key}) && is_array(${$data_key})) {
          $this->loaded_records = ${$data_key};
        }
      }
    }
  }

  /**
   * Return the merged contents of the records from the data store , and the values set by us.
   *
   * This is basically the data that would be written to the file if we were to write it right now.
   */
  function merged_records() {
    return array_merge($this->loaded_records, $this->records);
  }

  /**
   * Expose the merged records to the template file.
   */
  function process() {
    $this->data['records'] = array_filter(array_merge($this->loaded_records, $this->records));
  }
}

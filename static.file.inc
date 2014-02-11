<?php

/**
 * @file
 * The file manipulation functions for the static module.
 *
 * These are all the functions necessary to manipulate the static cache.
 */

/**
 * Create a redirect page.
 */
function static_redirect_page($oldfile, $newfile) {
  // This is a simple html page that redirects to the new page.
  $data = '<html>' . PHP_EOL
      . '  <head>' . PHP_EOL
      . '    <meta http-equiv="refresh" content="0;URL=/' . $newfile . '">' . PHP_EOL
      . '  </head>' . PHP_EOL
      . '  <body>' . PHP_EOL
      . '    <a href="/' . $newfile . '">Page has moved to this location.</a>' . PHP_EOL
      . '  </body>' . PHP_EOL
      . '</html>' . PHP_EOL;

  // Create directory if it doesn't exist.
  $directory = dirname(static_get_normal_cache_dir() . '/' . $oldfile);
  if (!static_mkdir($directory)) {
    return FALSE;
  }
  // Save the redirect to the system.
  if (file_put_contents(static_get_normal_cache_dir() . '/' . $oldfile, $data) === FALSE) {
    watchdog('static', 'Could not create the file %file on your system', array('%file' => $oldfile), WATCHDOG_ERROR);
    return FALSE;
  }
}

/**
 * Write to a file. Ensures write is atomic via rename operation.
 */
function static_write_file($filename, $data, $chmod = 0644) {
  // Create directory if it doesn't exist.
  $directory = dirname($filename);
  if (!static_mkdir($directory)) {
    return FALSE;
  }

  // Save the file to the system.
  if (file_put_contents($filename, $data) === FALSE) {
    watchdog('static', 'Could not create the file %file on your system', array('%file' => $filename), WATCHDOG_ERROR);
    return FALSE;
  }

  // Chmod file so webserver can send it out.
  drupal_chmod($filename, $chmod);
  return TRUE;
}

/**
 * Create a directory.
 */
function static_mkdir($directory) {
  global $_static;

  // Only do something if it's not a dir.
  if (!is_dir($directory)) {
    if (!static_in_cache_dir($directory)) {
      return FALSE;
    }

    // Try to create the directory.
    $mode = variable_get('file_chmod_directory', 0775);
    if (!mkdir($directory, $mode, TRUE)) {
      watchdog('static', 'Could not create the directory %dir on your system', array('%dir' => $directory), WATCHDOG_ERROR);
      return FALSE;
    }
    drupal_chmod($directory);
  }
  return TRUE;
}

/**
 * Delete directory and everything under it.
 */
function static_delete_dir($dir) {
  $files = array_diff(scandir($dir), array('.', '..'));
  foreach ($files as $file) {
    if (is_link("$dir/$file") || is_file("$dir/$file")) {
      drupal_unlink("$dir/$file");
    }
    elseif (is_dir("$dir/$file")) {
      static_delete_dir("$dir/$file");
    }
  }
  return drupal_rmdir($dir);
}

/**
 * Returns the relative normal static dir. static/normal.
 */
function static_get_normal_cache_dir() {
  return variable_get('static_root_cache_dir', STATIC_ROOT_CACHE_DIR) . '/' . variable_get('static_normal_dir', STATIC_NORMAL_DIR);
}

/**
 * Verify that the operation is going to operate in the cache dir.
 */
function static_in_cache_dir($file) {
  global $_static;

  $good = TRUE;
  $real_file = realpath($file);
  $cache_dir = variable_get('static_root_cache_dir', STATIC_ROOT_CACHE_DIR);
  $real_cache_dir = realpath($cache_dir);

  // Only operate in the cache dir.
  // Check the real path.
  if (strpos($file, $cache_dir) !== 0
      || ($real_file && $real_cache_dir && strpos($real_file, $real_cache_dir) !== 0)
        ) {
    $good = FALSE;
  }

  // Send error to watchdog.
  if (!$good) {
    watchdog('static', 'An operation outside of the cache directory was attempted on your system. %file or %real_file is outside the cache directory %cache or %real_cache.', array(
      '%file'       => $file,
      '%real_file'  => $real_file,
      '%cache'      => $cache_dir,
      '%real_cache' => $real_cache_dir,
      ),
    WATCHDOG_CRITICAL);
  }

  return $good;
}

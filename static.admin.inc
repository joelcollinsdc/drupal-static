<?php

/**
 * @file
 * Admin page callbacks for the static module.
 */

/**
 * Form builder; Configure static settings.
 *
 * @ingroup forms
 * @see system_settings_form()
 */
function static_admin_settings() {
  $form['static_root_cache_dir'] = array(
    '#type' => 'textfield',
    '#title' => t('Root cache directory'),
    '#default_value' => variable_get('static_root_cache_dir', STATIC_ROOT_CACHE_DIR),
  );

  $form['static_normal_dir'] = array(
    '#type' => 'textfield',
    '#title' => t('Normal cache directory'),
    '#default_value' => variable_get('static_normal_dir', STATIC_NORMAL_DIR),
  );

  $form['static_cron'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable the cron static generator'),
    '#default_value' => variable_get('static_cron', TRUE),
    '#description'   => t("Use cron to generate static pages on the site."),
  );

  $form['static_dynamic_cache_time'] = array(
    '#type' => 'select',
    '#title' => t('Dynamic Cache Refresh'),
    '#default_value' => variable_get('static_dynamic_cache_time', 300),
    '#options' => array(
      0 => 'always',
      60 => '1 minute',
      300 => '5 minutes',
      600 => '10 minutes',
      1800 => '30 minutes',
      3600 => '1 hour',
      7200 => '2 hours',
      18000 => '5 hours',
      43200 => '12 hours',
      86400 => '1 day',
      -1 => 'never',
    ),
    '#description'   => t("Set how often dynamic pages should be regenerated with cron."),
  );

  $form['static_queue_seconds'] = array(
    '#type' => 'textfield',
    '#title' => t('Time (in seconds) to use for generating static pages on every cron run'),
    '#default_value' => variable_get('static_queue_seconds', 30),
    '#description'   => t("If this it too long, it may cause problems with other tasks that run during the cron runs."),
  );
  
  $form['generate'] = array(
    '#type' => 'fieldset',
    '#tree' => FALSE,
    '#title' => t('Generate now'),
  );
  
  $form['generate']['static_batch_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of items per batch run'),
    '#default_value' => variable_get('static_batch_size', STATIC_DEFAULT_CRON_LIMIT),
    '#size' => 4,
  );
  $form['generate']['button'] = array(
    '#type' => 'submit',
    '#value' => t('Generate now'),
  );

  $form['generate']['regenerate'] = array(
    '#type' => 'submit',
    '#value' => t('Queue all items for regeneration'),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  $form['generate']['clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear all generated data'),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  $form['#submit'][] = 'static_admin_settings_submit';
  
  return system_settings_form($form);
}

/**
 * Form validation handler for static_admin_settings().
 */
function static_admin_settings_validate(array &$form, array &$form_state) {
  $values = $form_state['values'];
  $all_lower = drupal_strtolower($values['all']);
  if (in_array($values['op'], array(t('Generate now'), t('Save configuration')))) {
    foreach (array('static_batch_size') as $field) {
      $val = trim($values[$field]);
      if (drupal_strtolower($val) == $all_lower) {
        $val = -1;
      }
      elseif (!$val || !is_numeric($val) || ((int) $val) != $val) {
        form_error($form['generate'][$field], t('Enter a non-zero integer. Use "-1" or "@all" for "all items".', array('@all' => $values['all'])));
      }
      else {
        $val = (int) $val;
      }
      $form_state['values'][$field] = $val;
    }
  }
  else {
    // For regenerate and clear, don't call system_settings_form_submit.
    foreach($form['#submit'] as $id => $callback) {
      if ($callback == 'system_settings_form_submit') {
        unset($form['#submit'][$id]);
      }
    }
  }
}

/**
 * Form submission handler for static_admin_index_status_form().
 *
 * @see static_admin_index_status_form_validate()
 */
function static_admin_settings_submit(array $form, array &$form_state) {
  $values = $form_state['values'];
  $form_state['redirect'] = 'admin/config/system/static';

  switch ($values['op']) {
    case t('Generate now'):
      if (!_static_batch_generating_create($values['static_batch_size'])) {
        drupal_set_message(t("Couldn't create a batch, please check the batch size."), 'warning');
      }
      break;

    case t('Queue all items for regeneration'):
      $form_state['redirect'] .= '/regenerate';
      break;

    case t('Clear all generated data'):
      $form_state['redirect'] .= '/clear';
      break;
  }
}

/**
 * Form constructor for a generic confirmation form.
 *
 * @param $type
 *   The type of entity (not the real "entity type"). Either "server" or
 *   "index".
 * @param $action
 *   The action that would be executed for this entity after confirming. One of
 *   "reindex" ("index" type only), "clear", "disable" or "delete".
 * @param Entity $entity
 *   The entity for which the action would be performed. Must have a "name"
 *   property.
 *
 * @return array|false
 *   Either a form array, or FALSE if this combination of type and action is
 *   not supported.
 */
function static_admin_confirm(array $form, array &$form_state, $action) {
  switch ($action) {
    case 'regenerate':
      $text = array(
        t('Re-generate static pages'),
        t('Do you really want to queue all pages for re-generation?'),
        t('This will mark all items to be regenerated. Old files will still exist until the new ones are regenerated.'),
        t('The pages were successfully marked for re-generation.'),
      );
      break;

    case 'clear':
      $text = array(
        t('Clear static cache'),
        t('Do you really want to clear the entire static cache?'),
        t('This will remove all files that are currently generated. Before the pages is regenerated, there won\' be any files if the files are publicly visible. This action cannot be undone.'),
        t('The cache was successfully deleted.'),
      );
      break;
    default:
      return FALSE;
  }
  $form = array(
    'action' => array(
      '#type' => 'value',
      '#value' => $action,
    ),
    'message' => array(
      '#type' => 'value',
      '#value' => $text[3],
    ),
  );
  $desc = "<h3>{$text[1]}</h3><p>{$text[2]}</p>";
  return confirm_form($form, $text[0], "admin/config/system/static", $desc);
}

/**
 * Submit function for static_admin_confirm().
 */
function static_admin_confirm_submit(array $form, array &$form_state) {
  $form_state['redirect'] = 'admin/config/system/static';
  
  $values = $form_state['values'];

  $action = $values['action'];

  $function = "static_{$action}";
  if ($function()) {
    drupal_set_message($values['message']);
  }
  else {
    drupal_set_message(t('An error has occurred while performing the desired action. Check the logs for details.'), 'error');
  }
}

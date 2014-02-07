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
  
  $form['static_batch_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of items per batch run'),
    '#default_value' => variable_get('static_batch_size', STATIC_DEFAULT_CRON_LIMIT),
    '#size' => 4,
    '#description'   => t("This is the number of items to process per batch run. If too large, it may cause timeout issues."),
  );

  return system_settings_form($form);
}

/**
 * Page callback for static status
 */
function static_admin_status() {
  $status = array(
    STATIC_EXPIRED => 0,
    STATIC_CURRENT => 0,
    STATIC_ERROR => 0,
    'total' => 0,
  );
  $results = static_get_status();
  
  foreach($results as $result) {
    $status[$result->status] += $result->count;
    $status['total'] += $result->count;
  }
  
  $form['status'] = array(
    '#type' => 'fieldset',
    '#tree' => FALSE,
    '#title' => t('Status'),
  );
  
  $form['status']['current'] = array(
    '#markup' => t('@current items generated and up to date.', array('@current' => $status[STATIC_CURRENT])),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  $form['status']['expired'] = array(
    '#markup' => t('@expired items need generation.', array('@expired' => $status[STATIC_EXPIRED])),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  $form['status']['error'] = array(
    '#markup' => t('@error items did not generate properly.', array('@error' => $status[STATIC_ERROR])),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  $form['status']['total'] = array(
    '#markup' => t('@total total items.', array('@total' => $status['total'])),
    '#prefix' => '<div>',
    '#suffix' => '</div>',
  );
  
  $form['actions'] = array(
    '#type' => 'fieldset',
    '#tree' => FALSE,
    '#title' => t('Actions'),
  );
  
  if ($status[STATIC_EXPIRED] > 0) {
    $form['actions']['generate'] = array(
      '#type' => 'submit',
      '#value' => t('Generate now'),
    );
  }

  if ($status[STATIC_CURRENT] > 0 || $status[STATIC_ERROR] > 0) {
    $form['actions']['regenerate'] = array(
      '#type' => 'submit',
      '#value' => t('Queue all items for regeneration'),
    );
  }
  
  $form['actions']['clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear all generated data'),
  );
  
  return $form;
}

/**
 * Form submission handler for static_admin_index_status_form().
 *
 * @see static_admin_index_status_form_validate()
 */
function static_admin_status_submit(array $form, array &$form_state) {
  $values = $form_state['values'];
  $form_state['redirect'] = 'admin/config/system/static';

  switch ($values['op']) {
    case t('Generate now'):
      if (!_static_batch_generating_create(variable_get('static_batch_size', STATIC_DEFAULT_CRON_LIMIT))) {
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

/**
 * Page callback for static status
 */
function static_admin_paths() {
  $session = isset($_SESSION['static_path_filter']) ? $_SESSION['static_path_filter'] : array();
  
  $session += array(
    'status' => '',
    'path' => '',
    'alias' => '',
  );

  $form['filters'] = array(
    '#type' => 'fieldset',
    '#attributes' => array('class' => array('exposed-filters')),
    '#title' => t('Show only items where'),
  );
  
  $form['filters']['filters'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#attributes' => array('class' => array('filters')),
  );

  $form['filters']['filters']['status'] = array(
    '#type' => 'select',
    '#title' => 'status',
    '#options' => array(
      '' => 'any',
      STATIC_CURRENT => 'current',
      STATIC_EXPIRED => 'needs generation',
      STATIC_ERROR => 'error',
    ),
    '#default_value' => $session['status'],
  );
  
  $form['filters']['filters']['path'] = array(
    '#type' => 'textfield',
    '#title' => 'path',
    '#size' => 30,
    '#default_value' => $session['path'],
  );

  $form['filters']['filters']['alias'] = array(
    '#type' => 'textfield',
    '#title' => 'alias',
    '#size' => 30,
    '#default_value' => $session['alias'],
  );

  $form['filters']['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => count(array_filter($session)) ? t('Refine') : t('Filter'),
  );
  if (count(array_filter($session))) {
    $form['filters']['status']['actions']['undo'] = array('#type' => 'submit', '#value' => t('Undo'));
    $form['filters']['status']['actions']['reset'] = array('#type' => 'submit', '#value' => t('Reset'));
  }

  // Build the 'Update options' form.
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update options'),
    '#attributes' => array('class' => array('container-inline')),
  );
  $options = array();
  foreach (module_invoke_all('node_operations') as $operation => $array) {
    $options[$operation] = $array['label'];
  }
  $form['options']['operation'] = array(
    '#type' => 'select',
    '#title' => t('Operation'),
    '#title_display' => 'invisible',
    '#options' => $options,
    '#default_value' => 'approve',
  );
  $form['options']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#validate' => array('node_admin_nodes_validate'),
    '#submit' => array('node_admin_nodes_submit'),
  );

  // Build the sortable table header.
  $header = array(
    'path' => array('data' => t('Path'), 'field' => 's.path'),
    'alias' => array('data' => t('Alias'), 'field' => 's.alias'),
    'file' => array('data' => t('Generated File'), 'field' => 's.file'),
    'status' => array('data' => t('Status'), 'field' => 's.status'),
    'updated' => array('data' => t('Updated'), 'field' => 's.updated', 'sort' => 'desc')
  );
  $header['operations'] = array('data' => t('Operations'));

  $query = db_select('static', 's')
      ->extend('PagerDefault')
      ->extend('TableSort')
      ->fields('s');
  $options = array();
  foreach ($session as $index => $filter) {
    if ($filter == '') {
      continue;
    }
    switch ($index) {
      case 'status':
        $query->condition('s.' . $index, $filter);
      case 'path':
      case 'alias':
        $query->condition('s.' . $index, '%' . $filter . '%', 'LIKE');
        break;
    }
  }

  $infos = $query
    ->limit(50)
    ->orderByHeader($header)
    ->execute()
    ->fetchAll();

  $destination = drupal_get_destination();
  foreach ($infos as $info) {
    $options[$info->id] = array(
      'path' => check_plain($info->path),
      'alias' => check_plain($info->alias),
      'file' => check_plain($info->file),
      'status' => ($info->status ? ($info->status == 2 ? t('error') : t('current')) : t('needs generation')),
      'updated' => format_date($info->updated, 'short'),
    );
    // Build a list of all the accessible operations for the current node.
    $operations = array();
//    if (node_access('update', $node)) {
//      $operations['edit'] = array(
//        'title' => t('edit'),
//        'href' => 'node/' . $node->nid . '/edit',
//        'query' => $destination,
//      );
//    }
//    if (node_access('delete', $node)) {
//      $operations['delete'] = array(
//        'title' => t('delete'),
//        'href' => 'node/' . $node->nid . '/delete',
//        'query' => $destination,
//      );
//    }
    $options[$info->id]['operations'] = array();
    if (count($operations) > 1) {
      // Render an unordered list of operations links.
      $options[$info->id]['operations'] = array(
        'data' => array(
          '#theme' => 'links__node_operations',
          '#links' => $operations,
          '#attributes' => array('class' => array('links', 'inline')),
        ),
      );
    }
    elseif (!empty($operations)) {
      // Render the first and only operation as a link.
      $link = reset($operations);
      $options[$info->id]['operations'] = array(
        'data' => array(
          '#type' => 'link',
          '#title' => $link['title'],
          '#href' => $link['href'],
          '#options' => array('query' => $link['query']),
        ),
      );
    }
  }

  $form['items'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No content available.'),
  );

  $form['pager'] = array('#markup' => theme('pager'));
  return $form;
}

/**
 * Submit handler for static_admin_paths()
 */
function static_admin_paths_submit($form, &$form_state) {
  switch ($form_state['values']['op']) {
    case t('Filter'):
    case t('Refine'):
      $_SESSION['static_path_filter'] = $form_state['values']['filters'];
      break;
    case t('Reset'):
      $_SESSION['static_path_filter'] = array();
      break;
  }
}
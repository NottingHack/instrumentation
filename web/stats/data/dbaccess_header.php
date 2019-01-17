<?php

header('Access-Control-Allow-Origin: *');
header("Cache-Control: max-age=43200"); // Stats don't change much and are expensive to generate, so say are valid for 12 hours
header('Expires: '.gmdate('D, d M Y H:i:s \G\M\T', time() + 43200)); 
header("Content-type: application/json");

include "../../../www_secure/db.php";

function sql_to_tt_json($db, $sql, $params=null)
/* Given a PDO/MySQL connection and sql statment with optional parameters, return the results
 * in a JSON file suitable for Tidy-Table:
 *   https://labs.mbrooks.info/demos/Tidy-Table/
 */
{
  $stmt = $db->prepare($sql);
  $stmt->execute($params);
  $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
  
  // Generate the "columns" section of the JSON
  for ($i = 0; $i < $stmt->columnCount(); $i++) 
  {
      $col = $stmt->getColumnMeta($i);
      $output['columns'][] = $col['name'];
  }  
  
  // Now generate the "data" section
  $data = Array();
  foreach ($rows as $row)
  {
    $data_row = Array();
    foreach ( $output['columns'] as $col)
      $data_row[] = $row[$col];

    $data[] = $data_row;
  }
  $output['data'] = $data;
   
  return json_encode($output);
}

function sql_to_json($db, $sql)
{
  $stmt = $db->prepare($sql);
  $stmt->execute();
  $rows = $stmt->fetchAll(PDO::FETCH_ASSOC);
  return json_encode($rows);
}
?>
<?php
require "../../CNHDBAccess.php";

header('Access-Control-Allow-Origin: *');  
header("Cache-Control: no-cache, must-revalidate");
header("Content-type: application/json");

$oInstDB = new CNHDBAccess('localhost', 'nh-web', 'nh-web', 'instrumentation');
$oInstDB->sp_get_space_status($space_open, $last_change, $rs_temps);

$aStatus = Array();
$aStatus['api'] = "0.12";
$aStatus['space'] = "Nottinghack";
$aStatus['logo'] = "http://lspace.nottinghack.org.uk/status/logo.png";
$aStatus['icon'] = array("open" => "http://lspace.nottinghack.org.uk/status/logo_open.png", "closed" => "http://lspace.nottinghack.org.uk/status/logo_closed.png");
$aStatus['url'] = "http://www.nottinghack.org.uk";
$aStatus['address'] = "Unit F6, Roden House Business Centre, Alfred Street South, Nottingham, NG3 1JH";
$aStatus['contact'] = array("irc" => "irc://freenode/#nottinghack", "twitter" => "@hsnotts", "ml" => "http://groups.google.com/group/nottinghack");
$aStatus['lat'] = 52.9557;
$aStatus['lon'] = -1.1350;
$aStatus['open'] = space_state();
$aStatus['lastchange'] = intval($last_change);
$aStatus['sensors'] = space_temps();
echo json_encode($aStatus);
echo "\n";


function space_state()
{
  global $space_open;
  switch ($space_open)
  {
    case "Yes":
      return true;
      
    case "No":
      return false;
  }
}

function space_temps()
{
  global $rs_temps;
  
  $aSensors = array();
  $aTemps = array();
 
  foreach ($rs_temps as $row) 
    $aTemps[$row['sensor']] = $row['temp'];
    
 
  $aSensors['temp'] =  $aTemps;
  
  return array($aSensors);
}
?>
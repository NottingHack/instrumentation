<?php
require "../../db.php";

header('Access-Control-Allow-Origin: *');
header("Cache-Control: no-cache, must-revalidate");
header("Content-type: application/json");

//$oInstDB = new CNHDBAccess('localhost', 'nh-web', 'nh-web', 'instrumentation');
$oInstDB = db_link();
$oInstDB->sp_get_space_status($space_open, $last_change, $rs_temps);

$aStatus = Array();
$aStatus['api'] = "0.13";
$aStatus['space'] = "Nottinghack";
$aStatus['logo'] = "http://lspace.nottinghack.org.uk/status/logo.png";
$aStatus['icon'] = array("open" => "http://lspace.nottinghack.org.uk/status/logo_open.png", "closed" => "http://lspace.nottinghack.org.uk/status/logo_closed.png");
$aStatus['url'] = "http://www.nottinghack.org.uk";

$aStatus['location']['address'] =  "Unit F6, Roden House Business Centre, Alfred Street South, Nottingham, NG3 1JH";
$aStatus['location']['lat'] = 52.9557;
$aStatus['location']['lon'] = -1.1350;


$aStatus['contact'] = array("irc" => "irc://freenode/#nottinghack", "twitter" => "@hsnotts", "ml" => "http://groups.google.com/group/nottinghack");
$aStatus['contact']['irc'] = "irc://chat.freenode.net/#nottinghack";
$aStatus['contact']['twitter'] = "@hsnotts";
$aStatus['contact']['ml'] = "nottinghack@googlegroups.com";
$aStatus['contact']['facebook'] = "nottinghack";
$aStatus['contact']['issue_mail'] = "realm-admin@nottinghack.org.uk";

$aStatus['issue_report_channels'] = array("issue_mail");

$aStatus['open'] = $aStatus['state']['open'] = space_state();
$aStatus['state']['lastchange'] = intval($last_change);
$aStatus['state']['icon']['open'] =  "http://lspace.nottinghack.org.uk/status/logo_open.png";
$aStatus['state']['icon']['closed'] =  "http://lspace.nottinghack.org.uk/status/logo_closed.png";
$oInstDB->sp_space_net_activity($status_message);
$aStatus['state']['message'] = $status_message;

$aStatus['spacefed']['spacenet'] = true;
$aStatus['spacefed']['spacesaml'] = false;
$aStatus['spacefed']['spacephone'] = false;

$aStatus['feeds']['blog']['type'] = 'rss';
$aStatus['feeds']['blog']['url'] = 'http://planet.nottinghack.org.uk/rss20.xml';

$aStatus['feeds']['calendar']['type'] = 'ical';
$aStatus['feeds']['calendar']['url'] = 'https://www.google.com/calendar/ical/info%40nottinghack.org.uk/public/basic.ics';


$aStatus['sensors']['temperature'] = space_temps();
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

  $aTemps = array();

  foreach ($rs_temps as $row) 
    $aTemps[] = space_temp($row);

  return $aTemps;
}

function space_temp($row)
{
  $oTmp['value'] = floatval($row['temp']);
  $oTmp['unit'] = "°C";
  $oTmp['location'] = $row['sensor'];

  return $oTmp;
}

?>

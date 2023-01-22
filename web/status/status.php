<?php
require '../../www_secure/db.php';

header('Access-Control-Allow-Origin: *');
header('Cache-Control: no-cache, must-revalidate');
header('Content-type: application/json');

$oInstDB = db_link();
$oInstDB->sp_get_space_status($space_open, $last_change, $rs_temps);

$aStatus = [];
$aStatus['api'] = '0.13';
$aStatus['api_compatibility'] = [
  '14',
];
$aStatus['space'] = 'Nottinghack';
$aStatus['logo'] = 'https://lspace.nottinghack.org.uk/status/logo.png';
$aStatus['icon'] = [
  'open' => 'https://lspace.nottinghack.org.uk/status/logo_open.png',
  'closed' => 'https://lspace.nottinghack.org.uk/status/logo_closed.png',
];
$aStatus['url'] = 'http://www.nottinghack.org.uk';

$aStatus['location'] = [
  'address' => 'Unit F6, Roden House Business Centre, Alfred Street South, Nottingham, NG3 1JH',
  'lat' => 52.9557,
  'lon' => -1.1350,
  'timezone' => 'Europe/London',
];

$aStatus['contact'] = [
  'irc' => 'ircs://irc.libera.chat:6697/#nottinghack',
  'twitter' => '@hsnotts',
  'ml' => 'nottinghack@googlegroups.com',
  'facebook' => 'nottinghack',
  'issue_mail' => base64_encode('realm-admin@nottinghack.org.uk'),
];

// 13
$aStatus['issue_report_channels'] = [
  'issue_mail'
];

$oInstDB->sp_space_net_activity($status_message);
$aStatus['state'] = [
  'open' => space_state(),
  'lastchange' => intval($last_change),
  'icon' => [
    'open' =>  'https://lspace.nottinghack.org.uk/status/logo_open.png',
    'closed' =>  'https://lspace.nottinghack.org.uk/status/logo_closed.png',
  ],
  'message' => $status_message,
];

$aStatus['spacefed'] = [
  'spacenet' => true,
  'spacesaml' => false,
  'spacephone' => false,
];

$aStatus['feeds'] = [
  'blog' => [
    'type' => 'rss',
    'url' => 'http://planet.nottinghack.org.uk/rss20.xml',
  ],
  'calendar' => [
    'type' => 'ical',
    'url' => 'https://www.google.com/calendar/ical/info%40nottinghack.org.uk/public/basic.ics',
  ],
];

$aStatus['sensors'] = [
  'temperature' => space_temps(),
  // 'total_member_count' => [
  //   'value' => ??,
  // ],
];
echo json_encode($aStatus);
echo "\n";


function space_state()
{
  global $space_open;
  switch ($space_open)
  {
    case 'Yes':
      return true;

    case 'No':
    default:
      return false;
  }
}

function space_temps()
{
  global $rs_temps;

  $aTemps = [];

  foreach ($rs_temps as $row) 
    $aTemps[] = space_temp($row);

  return $aTemps;
}

function space_temp($row)
{
  return [
    'value' => floatval($row['temp']),
    'unit' => '°C',
    'location' => $row['sensor'],
  ];
}

?>


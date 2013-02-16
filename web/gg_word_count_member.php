<html>
<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />

  <body>
    <table border="0" cellspacing="2" cellpadding="2">

<?php
require "db.php";
//require "../db.php";

$date_from = $_GET["from"];
$date_to   = $_GET["to"];
$ggaddresses_id = $_GET["ggaddresses_id"];

if ((!is_numeric($date_from)) || (!is_numeric($date_to)) || (!is_numeric($ggaddresses_id)))
  die("Invalid input");

$oInstDB = db_link();

$sSQL = "
  select 
    ga.ggaddress_name as name, 
    FROM_UNIXTIME($date_from) as date_from,
    FROM_UNIXTIME($date_to)   as date_to,
    count(*) as total_emails, 
    sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) as total_wc,
    sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) / count(*) as avg_wc_per_email
  from gg_summary gs
  inner join gg_addresses ga 
    on gs.ggaddresses_id = ga.ggaddresses_id
  where gs.ggemail_date >= FROM_UNIXTIME($date_from)
    and gs.ggemail_date <  FROM_UNIXTIME($date_to)
    and gs.ggaddresses_id = $ggaddresses_id
";

if ($oResult = $oInstDB->query($sSQL))
{
  if ($oRow =  $oResult->fetch_assoc())
  {
    echo "\n";
    echo "<tr><td>Name:               </td><td>". $oRow['name'] .             "</td></tr>\n";
    echo "<tr><td>From date:          </td><td>". $oRow['date_from'] .        "</td></tr>\n";
    echo "<tr><td>To date:            </td><td>". $oRow['date_to'] .          "</td></tr>\n";
    echo "<tr><td>Total emails:       </td><td>". $oRow['total_emails'] .     "</td></tr>\n";
    echo "<tr><td>Total word count:   </td><td>". $oRow['total_wc'] .         "</td></tr>\n";
    echo "<tr><td>Average word count: </td><td>". $oRow['avg_wc_per_email'] . "</td></tr>\n";
  }
  echo "</table>\n";
  $oResult->free();
} 

?>
    <table border="2" cellspacing="2" cellpadding="2">
    <tr>
      <td>Subject</td>
      <td>Date</td>
      <td>Auto word count</td>
      <td>Manual word count</td>
    </tr>

<?php


$sSQLVDetails = "
  select 
    gm.ggemail_id, 
    gm.ggemail_subj, 
    gm.ggemail_date, 
    gs.ggsum_auto_wc, 
    gs.ggsum_manual_wc
  from gg_email gm
  inner join gg_summary gs
    on gs.ggemail_id = gm.ggemail_id
  where gs.ggemail_date >= FROM_UNIXTIME($date_from)
    and gs.ggemail_date <  FROM_UNIXTIME($date_to)
    and gs.ggaddresses_id = $ggaddresses_id
";


if ($oResult = $oInstDB->query($sSQLVDetails))
{
  while ($oRow = $oResult->fetch_assoc()) 
  {
    echo "    <tr>\n";
  //echo "      <td>" . $oRow['ggemail_subj'] . "</td>\n";
    echo "      <td> <a href=\"gg_word_count_email.php?ggemail_id=" . $oRow['ggemail_id'] . "\">" . $oRow['ggemail_subj'] . "</a>\n"; 
    echo "      <td>" . $oRow['ggemail_date'] . "</td>\n";
    echo "      <td>" . $oRow['ggsum_auto_wc'] . "</td>\n";
    echo "      <td>" . $oRow['ggsum_manual_wc'] . "</td>\n";
    echo "    </tr>\n";
  }
  
  $oResult->free();
  $oInstDB->close();
} 
  
?>

</table>
</body>
</html>

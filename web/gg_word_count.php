<html>
<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />

  <body>

<?php

if (isset($_GET['from']))
{

?>
    <table border="0" cellspacing="2" cellpadding="2">

<?php
require "db.php";
//require "../db.php";

$date_from = strtotime($_GET["from"]);
$date_to   = strtotime($_GET["to"]);

if (!is_numeric($date_from) || !is_numeric($date_to))
  die("Invalid input");

$oInstDB = db_link();


// user_count, total_emails, total_list_members, 
// pc_members_posting, total_wc, avg wc per email, avg wv per user

$sSQL = "
  select 
    FROM_UNIXTIME($date_from) as date_from,
    FROM_UNIXTIME($date_to)   as date_to,
    count(*) as total_emails, 
    sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) as total_wc,
    sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) / count(*) as avg_wc_per_email
  from gg_summary gs
  where gs.ggemail_date >= FROM_UNIXTIME($date_from)
    and gs.ggemail_date <  FROM_UNIXTIME($date_to)
";

if ($oResult = $oInstDB->query($sSQL))
{
  if ($oRow =  $oResult->fetch_assoc())
  {
    echo "\n";
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
      <td>Person</td>
      <td>Number of emails sent to list</td>
      <td>User word count</td>
      <td>User average words per email</td>
    </tr>

<?php


$sSQLVDetails = "
  select 
    ga.ggaddress_name as name, 
    gs.ggaddresses_id,
    count(*) as email_count,
    sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) as word_count,
    sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) / count(*) as avg_words_per_email
  from gg_summary gs 
  inner join gg_addresses ga 
    on gs.ggaddresses_id = ga.ggaddresses_id
  where gs.ggemail_date >= FROM_UNIXTIME($date_from)
    and gs.ggemail_date <  FROM_UNIXTIME($date_to)
  group by ga.ggaddress_name, gs.ggaddresses_id
  order by sum(coalesce(gs.ggsum_auto_wc, gs.ggsum_manual_wc)) desc;
";

if ($oResult = $oInstDB->query($sSQLVDetails))
{
  while ($oRow = $oResult->fetch_assoc()) 
  {
    echo "    <tr>\n";
    echo "      <td> <a href=\"gg_word_count_member.php?from=$date_from&to=$date_to&ggaddresses_id=" . $oRow['ggaddresses_id'] . "\">" . $oRow['name'] . "</a>\n"; 
    echo "      <td>" . $oRow['email_count'] . "</td>\n";
    echo "      <td>" . $oRow['word_count'] . "</td>\n";
    echo "      <td>" . $oRow['avg_words_per_email'] . "</td>\n";
    echo "    </tr>\n";
  }
  
  $oResult->free();
  $oInstDB->close();
} 
  
?>

</table>

<?php
}
 else
{

?>

<form name="input" action="<?php echo $PHP_SELF;?>" method="get">
  From date: <input type="text" name="from" value="2013-02-05"><br />
  To name: <input type="text" name="to" value="<?php echo date("Y-m-d"); ?>"><br />
  <input type="submit" value="Submit">
</form> 

<?php
}
?>

</body>
</html>

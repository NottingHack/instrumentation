<html>
<hed>
<meta http-equiv="Content-Type" content="text/html;charset=utf-8" />
<link rel="stylesheet" type="text/css" href="gg_word_count.css" />
</head>
  <body>
    <table border="0" cellspacing="2" cellpadding="2">

<?php
require "db.php";
//require "../db.php";

$ggemail_id = $_GET["ggemail_id"];

if (!is_numeric($ggemail_id))
  die("Invalid input");

$oInstDB = db_link();

$sReplace_stx = uniqid('', true);
$sReplace_etx = uniqid('', true);

$sSQL = "
  select 
    ga.ggaddress_name,
    ge.ggemail_subj,
    gs.ggsum_manual_wc,
    gs.ggsum_auto_wc,
    ge.ggemail_body_wc,
    ge.ggemail_date
  from gg_email ge
  inner join gg_summary gs
    on gs.ggemail_id = ge.ggemail_id
  inner join gg_addresses ga 
    on gs.ggaddresses_id = ga.ggaddresses_id
  where ge.ggemail_id = $ggemail_id
";

if ($oResult = $oInstDB->query($sSQL))
{
  if ($oRow =  $oResult->fetch_assoc())
  {
    echo "\n";
    echo "<tr><td>Name:              </td><td>". $oRow['ggaddress_name'] .  "</td></tr>\n";
    echo "<tr><td>Date:              </td><td>". $oRow['ggemail_date'] .    "</td></tr>\n";
    echo "<tr><td>Subject            </td><td>". $oRow['ggemail_subj'] .    "</td></tr>\n";
    echo "<tr><td>Auto word count:   </td><td>". $oRow['ggsum_auto_wc'] .   "</td></tr>\n";
    echo "<tr><td>Manual word count: </td><td>". $oRow['ggsum_manual_wc'] . "</td></tr>\n";
  }
  echo "</table>\n";
  
  $email_body = $oRow['ggemail_body_wc'];
  echo "<br /><br />";
  
  
  $email_body = str_replace(chr(02), $sReplace_stx, $email_body);
  $email_body = str_replace(chr(03), $sReplace_etx, $email_body);  
  
  $email_body = htmlspecialchars($email_body, ENT_QUOTES | ENT_SUBSTITUTE);
  
  $email_body = str_replace($sReplace_stx, "<span class=\"wc_count\">", $email_body);
  $email_body = str_replace($sReplace_etx, "</span>", $email_body);
  echo "<pre>\n";
  echo $email_body;
  echo "</pre>\n";
  
  
  $oResult->free();
} 

?>

</body>
</html>

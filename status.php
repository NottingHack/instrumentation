<html>
<head>
<title>instrumentation status</title>
</head>
<body>
<?php
  $username="gk";
  $password="gk";
  $database="instrumentation";

  mysql_connect(localhost,$username,$password);
  mysql_select_db($database) or die( "Unable to select database");


  $query="select service_name, status, status_str, reply_time from service_status";
  $result=mysql_query($query);

  echo '<table border="1">';
  echo "<tr><td>Service</td><td>Status</td><td>Details</td><td>Last response</td></tr>\n";
  
  $i=0;
  while ($i < mysql_numrows($result)) 
  {
    
    if (mysql_result($result,$i,"status") == "1") 
      echo "<tr>\n";
    else      
      echo "<tr bgcolor=\"red\">\n";

    echo "<td>" . mysql_result($result,$i,"service_name") . "</td>\n";
    
    if (mysql_result($result,$i,"status") == "1") 
      echo "<td>Running</td>\n";
    else
      echo "<td>No response</td>\n";

    if (mysql_result($result,$i,"status_str") == "")
      echo "<td>(No response)</td>\n";
    else
      echo "<td>" . mysql_result($result,$i,"status_str") . "</td>\n";       
    

    if (mysql_result($result,$i,"reply_time") == "")
      echo "<td>(never)</td>\n";
    else
      echo "<td>" . mysql_result($result,$i,"reply_time") . "</td>\n";
    
    echo "</tr>\n";
    $i++;
  }

?>
</body>
</html>



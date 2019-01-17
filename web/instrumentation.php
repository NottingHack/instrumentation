<html>
<head>
<title>instrumentation status</title>
</head>
<body>
<?php
  require "../www_secure/db.php";

  $link = db_link2();

  $query="SELECT service_name, status, status_str, reply_time, restart_time FROM service_status ORDER BY service_name";
  $result=mysqli_query($link, $query);

  echo '<table border="1">';
  echo "<tr><td>Service</td><td>Status</td><td>Details</td><td>Last response</td><td>Last restart</td></tr>\n";


  while ($row = mysqli_fetch_assoc($result))
  {

  //  printf ("%s (%s)\n", $row["Name"], $row["CountryCode"]);

//while ($i < mysqli_numrows($result))
//{
    if ($row["status"] == "1")
      echo "<tr>\n";
    else
      echo "<tr bgcolor=\"red\">\n";

    echo "<td>" . $row["service_name"] . "</td>\n";

    if ($row["status"] == "1")
      echo "<td>Running</td>\n";
    else
      echo "<td>No response</td>\n";

    if ($row["status_str"] == "")
      echo "<td>(No response)</td>\n";
    else
      echo "<td>" . $row["status_str"] . "</td>\n";

    if ($row["reply_time"] == "")
      echo "<td>(never)</td>\n";
    else
      echo "<td>" . $row["reply_time"] . "</td>\n";

    if ($row["restart_time"] == "")
      echo "<td>(unknown)</td>\n";
    else
      echo "<td>" . $row["restart_time"] . "</td>\n";


    echo "</tr>\n";
  }

?>
</table>
</body>
</html>



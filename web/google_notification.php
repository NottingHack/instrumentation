<?php
set_error_handler("error_handler"); // on any error, just stop script

require "../www_secure/db.php";

writeMsg(print_r($_SERVER, true));



$channel_id  = $_SERVER['HTTP_X_GOOG_CHANNEL_ID'];
$resource_id = $_SERVER['HTTP_X_GOOG_RESOURCE_ID'];


$link = db_link2();
$tool_name = get_tool_name($link, $channel_id, $resource_id);

if ($tool_name != "")
{
  $topic = "nh/bookings/poll";
  echo exec("mosquitto_pub -h 192.168.0.1 -t \"$topic\" -m \"$tool_name\"");
}

mysqli_close($link);


echo "\n";

function error_handler($errno, $errstr, $errfile, $errline)
{
  writeMsg("error_handler> errorno=[$errno] error=[$errstr] line=[$errline]");
  die();
}

function writeMsg($sMsg) 
{
  $sLogFile = "/home/instrumentation/logs/google.log";

  $sOutput = date("M d H:i:s") . ": " . $sMsg . "\n";
  file_put_contents($sLogFile, $sOutput, FILE_APPEND);
}


function get_tool_name($connection, $channel_id, $resource_id)
{
  $sql = "
    SELECT t.tool_name
    FROM tl_tools t
    INNER JOIN tl_google_notifications g ON g.tool_id = t.tool_id
    WHERE g.channel_id = ?
      AND g.resource_id = ?";

  if ($stmt = mysqli_prepare($connection, $sql))
  {
    mysqli_stmt_bind_param($stmt, "ss", $channel_id, $resource_id);
    if (mysqli_stmt_execute($stmt))
    {
      mysqli_stmt_bind_result($stmt, $tool_name);
      mysqli_stmt_fetch($stmt);
    } else
    {
      mysqli_stmt_close($stmt);
      return "";
    }
    mysqli_stmt_close($stmt);
  } else
  {
    return "";
  }

  return trim($tool_name);
}

?>


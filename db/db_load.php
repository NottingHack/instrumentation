<?php

/* Assumptions:
 *  1) For Stored Procedures, there is one SP per file, and the 
 *     filename is the same as the SP name (plus ".sql"), and all
 *     SPs are named "sp_...".
 * 
 *  2) For tables, there is one table per file, and the filename
 *     is "tb_<table name>.sql"
 */ 

$config_file = "database.conf";

$aConfig = parse_ini_file($config_file, true);
if (!$aConfig)
  die("Error reading config file [" . $config_file . "]!\n");

$mysql_server = $aConfig['mysql']['server'];
$mysql_dbase = $aConfig['mysql']['database'];
$mysql_user = $aConfig['mysql_admin']['username'];
$mysql_passwd = $aConfig['mysql_admin']['password'];

$oDB = new mysqli($mysql_server, $mysql_user, $mysql_passwd, $mysql_dbase);

if ($oDB->connect_errno) 
  die("Failed to connect to database: [" . $mysqli->connect_error . "]\n");


/* Is database empty? */
if (($tc = table_count($oDB)) === FALSE)
  die("Error counting tables: [" . $oDB->error . "]\n");
if ($tc > 0)
  $db_empty = FALSE;
else
  $db_empty = TRUE;

if (isset($argv[1]))
{
  $filename = $argv[1];
  
  if ($filename == "ALL")
  {
    if (!$db_empty)
    {
      echo "\nWARNING: database [$mysql_dbase] is not empty!!!\n";
      echo "Really drop/recreate all TABLES and SPs? (yes/no): ";
      if (fgets(STDIN) != "yes\n")
        exit();
    }
    $aFileList = scandir("./");
    foreach($aFileList as $file) 
    {
      if (is_sp($file) || is_table($file))
        load_file($file, $aConfig, $oDB);
    }    
  }
  else if (is_table($filename))
  {
    $table_name = str_ireplace("tb_" , "", $filename);
    $table_name = str_ireplace(".sql", "", $table_name);
    
    if (table_exists($oDB, $aConfig, $table_name))
    {
      echo "\nWARNING: table [$table_name] in [$mysql_dbase] exists!\n";
      echo "Drop/recreate? (yes/no): ";
      if (fgets(STDIN) != "yes\n")
        exit();
    }
    load_file($filename, $aConfig, $oDB);
  } 
  else if (is_sp($filename))
  {
    load_file($filename, $aConfig, $oDB);
  } else
  {
    // Not an SP, not a table. What is it?
    die("\nUnknown object type: [$filename]\n");
  }
}
else
{
  echo "\nNo file specified. Load all SPs? (yes/no): ";
  $input = fgets(STDIN);
  if ($input == "yes\n")
  {
    $aFileList = scandir("./");
    foreach($aFileList as $file) 
    {
      if (is_sp($file))
        load_file($file, $aConfig, $oDB);
    }
  }
}
  
  
function load_file($file, $aConfig, $oDB)
{
  $mysql_server = $aConfig['mysql']['server'];
  $mysql_dbase = $aConfig['mysql']['database'];
  $mysql_user = $aConfig['mysql_admin']['username'];
  $mysql_passwd = $aConfig['mysql_admin']['password'];
  $mysql_runtime_user = $aConfig['mysql_runtime']['username'];

  echo "Load [$file]...";
    
  $cmd = "mysql -u$mysql_user -p$mysql_passwd -h$mysql_server $mysql_dbase < $file";
  exec ($cmd, $out, $retval);    
  if ($retval != 0)
    die("\n>>FAILED<<\n");
  
  // Now grant execute permission to the runtime account
  if (is_sp($file))
  {
    $spname = str_ireplace(".sql", "", $file);
    $query = "GRANT EXECUTE ON PROCEDURE $spname TO '$mysql_runtime_user'@'localhost'";
    if ($oDB->query($query) === TRUE)
      echo "OK";
    else
    {
      echo "Error: [" . $oDB->error . "]\n";
      die(">>FAILED<<\n");
    }
  } else
    echo "OK";
  
  echo "\n";
}

function is_sp($filename)
{
  if (preg_match ("/sp_+[^\.]+.sql/", $filename) == 1)
    return TRUE;
  else
    return FALSE; 
}

function is_table($filename)
{
  if (preg_match ("/tb_+[^\.]+.sql/", $filename) == 1)
    return TRUE;
  else
    return FALSE; 
}


function table_count($oDB)
{
  if ($oResult = $oDB->query("show tables")) 
  {
    $num_rows = $oResult->num_rows;
    $oResult->close();
    return $num_rows;
  } else
    return FALSE;
}

function table_exists($oDB, $aConfig, $table_name)
{
  $bRet = TRUE;
  $mysql_dbase = $aConfig['mysql']['database'];
  $query = "select count(*) as c " .
           "from information_schema.tables " .
           "where table_schema = '$mysql_dbase' " . 
           "and table_name = '$table_name'";

  $oResult = $oDB->query($query);
  if($oResult)
  {
    $oRow = $oResult->fetch_object();
    {
      if ($oRow->c == 0)
        $bRet = FALSE;
    } 
    $oResult->close();
  } else
    die("Error checking if [$table_name] exists!\n");
  return $bRet;
}



?>